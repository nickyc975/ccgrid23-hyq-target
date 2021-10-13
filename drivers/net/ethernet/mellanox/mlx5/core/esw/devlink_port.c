// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/* Copyright (c) 2020 Mellanox Technologies Ltd. */

#include <linux/mlx5/driver.h>
#include <net/devlink.h>
#include "eswitch.h"
#include "mlx5_esw_devm.h"

#ifdef HAVE_DEVLINK_PORT_ATRRS_SET_GET_SUPPORT
static void
mlx5_esw_get_port_parent_id(struct mlx5_core_dev *dev, struct netdev_phys_item_id *ppid)
{
	u64 parent_id;

	parent_id = mlx5_query_nic_system_image_guid(dev);
	ppid->id_len = sizeof(parent_id);
	memcpy(ppid->id, &parent_id, sizeof(parent_id));
}
#endif

#ifdef HAVE_DEVLINK_PORT_TYPE_ETH_SET
static bool mlx5_esw_devlink_port_supported(struct mlx5_eswitch *esw, u16 vport_num)
{
	return vport_num == MLX5_VPORT_UPLINK ||
	       (mlx5_core_is_ecpf(esw->dev) && vport_num == MLX5_VPORT_PF) ||
	       mlx5_eswitch_is_vf_vport(esw, vport_num);
}

static struct devlink_port *mlx5_esw_dl_port_alloc(struct mlx5_eswitch *esw, u16 vport_num)
{
#ifdef HAVE_DEVLINK_PORT_ATRRS_SET_GET_SUPPORT
	struct mlx5_core_dev *dev = esw->dev;
#ifdef HAVE_DEVLINK_PORT_ATRRS_SET_GET_2_PARAMS
	struct devlink_port_attrs attrs = {};
#endif
	struct netdev_phys_item_id ppid = {};
	struct devlink_port *dl_port;
#if defined(HAVE_DEVLINK_PORT_ATTRS_PCI_PF_SET_CONTROLLER_NUM)
	u32 controller_num = 0;
	bool external;
#endif
	u16 pfnum;

	dl_port = kzalloc(sizeof(*dl_port), GFP_KERNEL);
	if (!dl_port)
		return NULL;

	mlx5_esw_get_port_parent_id(dev, &ppid);
	pfnum = PCI_FUNC(dev->pdev->devfn);
#if defined(HAVE_DEVLINK_PORT_ATTRS_PCI_PF_SET_CONTROLLER_NUM)
	if (external)
		controller_num = dev->priv.eswitch->offloads.host_number + 1;
#endif

	if (vport_num == MLX5_VPORT_UPLINK) {
#ifdef HAVE_DEVLINK_PORT_ATRRS_SET_GET_2_PARAMS
		attrs.flavour = DEVLINK_PORT_FLAVOUR_PHYSICAL;
		attrs.phys.port_number = pfnum;
		memcpy(attrs.switch_id.id, ppid.id, ppid.id_len);
		attrs.switch_id.id_len = ppid.id_len;
		devlink_port_attrs_set(dl_port, &attrs);
#else
		devlink_port_attrs_set(dl_port,
				DEVLINK_PORT_FLAVOUR_PHYSICAL,
				PCI_FUNC(dev->pdev->devfn),
				false, 0
#ifdef HAVE_DEVLINK_PORT_ATRRS_SET_GET_7_PARAMS
				,NULL, 0
#endif
		);
#endif
	} 
#ifdef HAVE_DEVLINK_PORT_ATTRS_PCI_PF_SET
	else if (vport_num == MLX5_VPORT_PF) {
		memcpy(dl_port->attrs.switch_id.id, ppid.id, ppid.id_len);
		dl_port->attrs.switch_id.id_len = ppid.id_len;
#if defined(HAVE_DEVLINK_PORT_ATTRS_PCI_PF_SET_GET_2_PARAMS)
		devlink_port_attrs_pci_pf_set(dl_port, pfnum);
#elif defined(HAVE_DEVLINK_PORT_ATTRS_PCI_PF_SET_4_PARAMS)
		devlink_port_attrs_pci_pf_set(dl_port,
				&ppid.id[0], ppid.id_len,
				pfnum);
#elif defined(HAVE_DEVLINK_PORT_ATTRS_PCI_PF_SET_CONTROLLER_NUM)
		devlink_port_attrs_pci_pf_set(dl_port, controller_num, pfnum, external);

#endif
	} else if (mlx5_eswitch_is_vf_vport(esw, vport_num)) {
		memcpy(dl_port->attrs.switch_id.id, ppid.id, ppid.id_len);
		dl_port->attrs.switch_id.id_len = ppid.id_len;
#if defined(HAVE_DEVLINK_PORT_ATTRS_PCI_VF_SET_GET_3_PARAMS)
		devlink_port_attrs_pci_vf_set(dl_port, pfnum, vport_num - 1);
#elif defined(HAVE_DEVLINK_PORT_ATTRS_PCI_VF_SET_GET_5_PARAMS)
		devlink_port_attrs_pci_vf_set(dl_port,
				&ppid.id[0], ppid.id_len,
				pfnum, vport_num - 1);
#elif defined(HAVE_DEVLINK_PORT_ATTRS_PCI_VF_SET_GET_CONTROLLER_NUM)
		devlink_port_attrs_pci_vf_set(dl_port, controller_num, pfnum,
					      vport_num - 1, external);
#endif
	}
#else
	else
                devlink_port_attrs_set(dl_port,
                                DEVLINK_PORT_FLAVOUR_VIRTUAL,
                                0, false , 0
#ifdef HAVE_DEVLINK_PORT_ATRRS_SET_GET_7_PARAMS
                                ,NULL, 0
#endif
		);
#endif /* HAVE_DEVLINK_PORT_ATTRS_PCI_PF_SET */
	return dl_port;
#else
	return NULL;
#endif /* HAVE_DEVLINK_PORT_ATRRS_SET_GET_SUPPORT */
}

static void mlx5_esw_dl_port_free(struct devlink_port *dl_port)
{
	kfree(dl_port);
}
#endif

int mlx5_esw_offloads_devlink_port_register(struct mlx5_eswitch *esw, u16 vport_num)
{
#ifdef HAVE_DEVLINK_PORT_TYPE_ETH_SET
	struct mlx5_core_dev *dev = esw->dev;
	struct devlink_port *dl_port;
	unsigned int dl_port_index;
	struct mlx5_vport *vport;
	struct devlink *devlink;
	int err;

	if (!mlx5_esw_devlink_port_supported(esw, vport_num))
		return 0;

	vport = mlx5_eswitch_get_vport(esw, vport_num);
	if (IS_ERR(vport))
		return PTR_ERR(vport);

	dl_port = mlx5_esw_dl_port_alloc(esw, vport_num);
	if (!dl_port)
		return -ENOMEM;

	devlink = priv_to_devlink(dev);
	dl_port_index = mlx5_esw_vport_to_devlink_port_index(dev, vport_num);
	err = devlink_port_register(devlink, dl_port, dl_port_index);
	if (err)
		goto reg_err;

	vport->dl_port = dl_port;
	return 0;

reg_err:
	mlx5_esw_dl_port_free(dl_port);
	return err;
#else
	return 0;
#endif
}

void mlx5_esw_offloads_devlink_port_unregister(struct mlx5_eswitch *esw, u16 vport_num)
{
#ifdef HAVE_DEVLINK_PORT_TYPE_ETH_SET
	struct mlx5_vport *vport;

	if (!mlx5_esw_devlink_port_supported(esw, vport_num))
		return;

	vport = mlx5_eswitch_get_vport(esw, vport_num);
	if (IS_ERR(vport))
		return;
	devlink_port_unregister(vport->dl_port);
	mlx5_esw_dl_port_free(vport->dl_port);
	vport->dl_port = NULL;
#endif
}

struct devlink_port *mlx5_esw_offloads_devlink_port(struct mlx5_eswitch *esw, u16 vport_num)
{
	struct mlx5_vport *vport;

	vport = mlx5_eswitch_get_vport(esw, vport_num);
	return vport->dl_port;
}

#if IS_ENABLED(CONFIG_MLXDEVM)
int mlx5_esw_devlink_sf_port_register(struct mlx5_eswitch *esw, struct devlink_port *dl_port,
				      u16 vport_num, u32 controller, u32 sfnum)
{
	return mlx5_devm_sf_port_register(esw->dev, vport_num, controller, sfnum);
}
#else
int mlx5_esw_devlink_sf_port_register(struct mlx5_eswitch *esw, struct devlink_port *dl_port,
				      u16 vport_num, u32 controller, u32 sfnum)
{
#ifdef HAVE_DEVLINK_PORT_ATTRS_PC_SF_SET
	struct mlx5_core_dev *dev = esw->dev;
	struct netdev_phys_item_id ppid = {};
	unsigned int dl_port_index;
	struct mlx5_vport *vport;
	struct devlink *devlink;
	u16 pfnum;
	int err;

	vport = mlx5_eswitch_get_vport(esw, vport_num);
	if (IS_ERR(vport))
		return PTR_ERR(vport);

	pfnum = PCI_FUNC(dev->pdev->devfn);
	mlx5_esw_get_port_parent_id(dev, &ppid);
	memcpy(dl_port->attrs.switch_id.id, &ppid.id[0], ppid.id_len);
	dl_port->attrs.switch_id.id_len = ppid.id_len;
	devlink_port_attrs_pci_sf_set(dl_port, controller, pfnum, sfnum, !!controller);
	devlink = priv_to_devlink(dev);
	dl_port_index = mlx5_esw_vport_to_devlink_port_index(dev, vport_num);
	err = devlink_port_register(devlink, dl_port, dl_port_index);
	if (err)
		return err;

	vport->dl_port = dl_port;
	return 0;
#else
	return -EOPNOTSUPP;
#endif
}
#endif

#if IS_ENABLED(CONFIG_MLXDEVM)
void mlx5_esw_devlink_sf_port_unregister(struct mlx5_eswitch *esw, u16 vport_num)
{
	mlx5_devm_sf_port_unregister(esw->dev, vport_num);
}
#else
void mlx5_esw_devlink_sf_port_unregister(struct mlx5_eswitch *esw, u16 vport_num)
{
#ifdef HAVE_DEVLINK_PORT_ATTRS_PC_SF_SET
	struct mlx5_vport *vport;

	vport = mlx5_eswitch_get_vport(esw, vport_num);
	if (IS_ERR(vport))
		return;
	devlink_port_unregister(vport->dl_port);
	vport->dl_port = NULL;
#endif
}
#endif
