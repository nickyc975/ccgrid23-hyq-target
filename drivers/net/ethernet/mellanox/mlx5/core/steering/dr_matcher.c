// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/* Copyright (c) 2019 Mellanox Technologies. */

#include "dr_types.h"

static bool dr_mask_is_smac_set(struct mlx5dr_match_spec *spec)
{
	return (spec->smac_47_16 || spec->smac_15_0);
}

static bool dr_mask_is_dmac_set(struct mlx5dr_match_spec *spec)
{
	return (spec->dmac_47_16 || spec->dmac_15_0);
}

static bool dr_mask_is_src_addr_set(struct mlx5dr_match_spec *spec)
{
	return (spec->src_ip_127_96 || spec->src_ip_95_64 ||
		spec->src_ip_63_32 || spec->src_ip_31_0);
}

static bool dr_mask_is_dst_addr_set(struct mlx5dr_match_spec *spec)
{
	return (spec->dst_ip_127_96 || spec->dst_ip_95_64 ||
		spec->dst_ip_63_32 || spec->dst_ip_31_0);
}

static bool dr_mask_is_ipv6_only_match_set(struct mlx5dr_match_spec *spec)
{
	return spec->src_ip_127_96 || spec->src_ip_95_64 ||
	       spec->src_ip_63_32 ||
	       spec->dst_ip_127_96 || spec->dst_ip_95_64 ||
	       spec->dst_ip_63_32;
}

static bool dr_mask_is_l3_base_set(struct mlx5dr_match_spec *spec)
{
	return (spec->ip_protocol || spec->frag || spec->tcp_flags ||
		spec->ip_ecn || spec->ip_dscp);
}

static bool dr_mask_is_tcp_udp_base_set(struct mlx5dr_match_spec *spec)
{
	return (spec->tcp_sport || spec->tcp_dport ||
		spec->udp_sport || spec->udp_dport);
}

static bool dr_mask_is_ipv4_set(struct mlx5dr_match_spec *spec)
{
	return (spec->dst_ip_31_0 || spec->src_ip_31_0);
}

static bool dr_mask_is_ipv4_5_tuple_set(struct mlx5dr_match_spec *spec)
{
	return (dr_mask_is_l3_base_set(spec) ||
		dr_mask_is_tcp_udp_base_set(spec) ||
		dr_mask_is_ipv4_set(spec));
}

static bool dr_mask_is_eth_l2_tnl_set(struct mlx5dr_match_misc *misc)
{
	return misc->vxlan_vni;
}

static bool dr_mask_is_ttl_set(struct mlx5dr_match_spec *spec)
{
	return spec->ttl_hoplimit;
}

#define DR_MASK_IS_L2_DST(_spec, _misc, _inner_outer) (_spec.first_vid || \
	(_spec).first_cfi || (_spec).first_prio || (_spec).cvlan_tag || \
	(_spec).svlan_tag || (_spec).dmac_47_16 || (_spec).dmac_15_0 || \
	(_spec).ethertype || (_spec).ip_version || \
	(_misc)._inner_outer##_second_vid || \
	(_misc)._inner_outer##_second_cfi || \
	(_misc)._inner_outer##_second_prio || \
	(_misc)._inner_outer##_second_cvlan_tag || \
	(_misc)._inner_outer##_second_svlan_tag)

#define DR_MASK_IS_ETH_L4_SET(_spec, _misc, _inner_outer) ( \
	dr_mask_is_l3_base_set(&(_spec)) || \
	dr_mask_is_tcp_udp_base_set(&(_spec)) || \
	dr_mask_is_ttl_set(&(_spec)) || \
	(_misc)._inner_outer##_ipv6_flow_label)

#define DR_MASK_IS_ETH_L4_MISC_SET(_misc3, _inner_outer) ( \
	(_misc3)._inner_outer##_tcp_seq_num || \
	(_misc3)._inner_outer##_tcp_ack_num)

#define DR_MASK_IS_FIRST_MPLS_SET(_misc2, _inner_outer) ( \
	(_misc2)._inner_outer##_first_mpls_label || \
	(_misc2)._inner_outer##_first_mpls_exp || \
	(_misc2)._inner_outer##_first_mpls_s_bos || \
	(_misc2)._inner_outer##_first_mpls_ttl)

static bool dr_mask_is_tnl_gre_set(struct mlx5dr_match_misc *misc)
{
	return (misc->gre_key_h || misc->gre_key_l ||
		misc->gre_protocol || misc->gre_c_present ||
		misc->gre_k_present || misc->gre_s_present);
}

#define DR_MASK_IS_OUTER_MPLS_OVER_GRE_SET(_misc) (\
	(_misc)->outer_first_mpls_over_gre_label || \
	(_misc)->outer_first_mpls_over_gre_exp || \
	(_misc)->outer_first_mpls_over_gre_s_bos || \
	(_misc)->outer_first_mpls_over_gre_ttl)

#define DR_MASK_IS_OUTER_MPLS_OVER_UDP_SET(_misc) (\
	(_misc)->outer_first_mpls_over_udp_label || \
	(_misc)->outer_first_mpls_over_udp_exp || \
	(_misc)->outer_first_mpls_over_udp_s_bos || \
	(_misc)->outer_first_mpls_over_udp_ttl)

static bool
dr_mask_is_vxlan_gpe_set(struct mlx5dr_match_misc3 *misc3)
{
	return (misc3->outer_vxlan_gpe_vni ||
		misc3->outer_vxlan_gpe_next_protocol ||
		misc3->outer_vxlan_gpe_flags);
}

static bool
dr_matcher_supp_vxlan_gpe(struct mlx5dr_cmd_caps *caps)
{
	return (caps->sw_format_ver == MLX5_HW_CONNECTX_6DX) ||
	       (caps->flex_protocols & MLX5_FLEX_PARSER_VXLAN_GPE_ENABLED);
}

static bool
dr_mask_is_tnl_vxlan_gpe(struct mlx5dr_match_param *mask,
			 struct mlx5dr_domain *dmn)
{
	return dr_mask_is_vxlan_gpe_set(&mask->misc3) &&
	       dr_matcher_supp_vxlan_gpe(&dmn->info.caps);
}

static bool dr_mask_is_tnl_geneve_set(struct mlx5dr_match_misc *misc)
{
	return misc->geneve_vni ||
	       misc->geneve_oam ||
	       misc->geneve_protocol_type ||
	       misc->geneve_opt_len;
}

static bool dr_mask_is_tnl_geneve_tlv_option(struct mlx5dr_match_misc3 *misc3)
{
	return misc3->geneve_tlv_option_0_data;
}

static bool
dr_matcher_supp_tnl_geneve(struct mlx5dr_cmd_caps *caps)
{
	return (caps->sw_format_ver == MLX5_HW_CONNECTX_6DX) ||
	       (caps->flex_protocols & MLX5_FLEX_PARSER_GENEVE_ENABLED);
}

static bool
dr_mask_is_tnl_geneve(struct mlx5dr_match_param *mask,
		      struct mlx5dr_domain *dmn)
{
	return dr_mask_is_tnl_geneve_set(&mask->misc) &&
	       dr_matcher_supp_tnl_geneve(&dmn->info.caps);
}

static int dr_matcher_supp_icmp_v4(struct mlx5dr_cmd_caps *caps)
{
	return (caps->sw_format_ver == MLX5_HW_CONNECTX_6DX) ||
	       (caps->flex_protocols & MLX5_FLEX_PARSER_ICMP_V4_ENABLED);
}

static int dr_matcher_supp_icmp_v6(struct mlx5dr_cmd_caps *caps)
{
	return (caps->sw_format_ver == MLX5_HW_CONNECTX_6DX) ||
	       (caps->flex_protocols & MLX5_FLEX_PARSER_ICMP_V6_ENABLED);
}

static bool dr_mask_is_icmpv6_set(struct mlx5dr_match_misc3 *misc3)
{
	return (misc3->icmpv6_type || misc3->icmpv6_code ||
		misc3->icmpv6_header_data);
}

static bool dr_mask_is_icmp(struct mlx5dr_match_param *mask,
			    struct mlx5dr_domain *dmn)
{
	if (DR_MASK_IS_ICMPV4_SET(&mask->misc3))
		return dr_matcher_supp_icmp_v4(&dmn->info.caps);
	else if (dr_mask_is_icmpv6_set(&mask->misc3))
		return dr_matcher_supp_icmp_v6(&dmn->info.caps);

	return false;
}

static bool dr_mask_is_wqe_metadata_set(struct mlx5dr_match_misc2 *misc2)
{
	return misc2->metadata_reg_a;
}

static bool dr_mask_is_reg_c_0_3_set(struct mlx5dr_match_misc2 *misc2)
{
	return (misc2->metadata_reg_c_0 || misc2->metadata_reg_c_1 ||
		misc2->metadata_reg_c_2 || misc2->metadata_reg_c_3);
}

static bool dr_mask_is_reg_c_4_7_set(struct mlx5dr_match_misc2 *misc2)
{
	return (misc2->metadata_reg_c_4 || misc2->metadata_reg_c_5 ||
		misc2->metadata_reg_c_6 || misc2->metadata_reg_c_7);
}

static bool dr_mask_is_gvmi_or_qpn_set(struct mlx5dr_match_misc *misc)
{
	return (misc->source_sqn || misc->source_port);
}

static bool dr_mask_is_flex_parser_id_0_3_set(u32 flex_parser_id,
					      u32 flex_parser_value)
{
	if (flex_parser_id)
		return flex_parser_id < 4;

	/* Using flex_parser 0 means that id is zero, thus value must be set */
	return flex_parser_value;
}

static bool dr_mask_is_flex_parser_0_3_set(struct mlx5dr_match_misc4 *misc4)
{
	return (dr_mask_is_flex_parser_id_0_3_set(
			misc4->prog_sample_field_id_0,
			misc4->prog_sample_field_value_0) ||
		dr_mask_is_flex_parser_id_0_3_set(
			misc4->prog_sample_field_id_1,
			misc4->prog_sample_field_value_1) ||
		dr_mask_is_flex_parser_id_0_3_set(
			misc4->prog_sample_field_id_2,
			misc4->prog_sample_field_value_2) ||
		dr_mask_is_flex_parser_id_0_3_set(
			misc4->prog_sample_field_id_3,
			misc4->prog_sample_field_value_3));
}

static bool dr_mask_is_flex_parser_id_4_7_set(u32 flex_parser_id)
{
	return flex_parser_id >= 4 && flex_parser_id < 8;
}

static bool dr_mask_is_flex_parser_4_7_set(struct mlx5dr_match_misc4 *misc4)
{
	return (dr_mask_is_flex_parser_id_4_7_set(misc4->prog_sample_field_id_0) ||
		dr_mask_is_flex_parser_id_4_7_set(misc4->prog_sample_field_id_1) ||
		dr_mask_is_flex_parser_id_4_7_set(misc4->prog_sample_field_id_2) ||
		dr_mask_is_flex_parser_id_4_7_set(misc4->prog_sample_field_id_3));
}

static int dr_matcher_supp_tnl_mpls_over_gre(struct mlx5dr_cmd_caps *caps)
{
	return caps->flex_protocols & MLX5_FLEX_PARSER_MPLS_OVER_GRE_ENABLED;
}

static bool dr_mask_is_tnl_mpls_over_gre(struct mlx5dr_match_param *mask,
					 struct mlx5dr_domain *dmn)
{
	return DR_MASK_IS_OUTER_MPLS_OVER_GRE_SET(&mask->misc2) &&
	       dr_matcher_supp_tnl_mpls_over_gre(&dmn->info.caps);
}

static int dr_matcher_supp_tnl_mpls_over_udp(struct mlx5dr_cmd_caps *caps)
{
	return caps->flex_protocols & MLX5_FLEX_PARSER_MPLS_OVER_UDP_ENABLED;
}

static bool dr_mask_is_tnl_mpls_over_udp(struct mlx5dr_match_param *mask,
					 struct mlx5dr_domain *dmn)
{
	return DR_MASK_IS_OUTER_MPLS_OVER_UDP_SET(&mask->misc2) &&
	       dr_matcher_supp_tnl_mpls_over_udp(&dmn->info.caps);
}

static bool dr_matcher_is_mask_consumed(struct mlx5dr_match_param *mask)
{
	int i;

	for (i = 0; i < sizeof(struct mlx5dr_match_param); i++)
		if (((u8 *)mask)[i] != 0)
			return false;

	return true;
}

static void dr_matcher_copy_mask(struct mlx5dr_match_param *dst_mask,
				 struct mlx5dr_match_param *src_mask,
				 u8 match_criteria)
{
	if (match_criteria & DR_MATCHER_CRITERIA_OUTER)
		dst_mask->outer = src_mask->outer;

	if (match_criteria & DR_MATCHER_CRITERIA_MISC)
		dst_mask->misc = src_mask->misc;

	if (match_criteria & DR_MATCHER_CRITERIA_INNER)
		dst_mask->inner = src_mask->inner;

	if (match_criteria & DR_MATCHER_CRITERIA_MISC2)
		dst_mask->misc2 = src_mask->misc2;

	if (match_criteria & DR_MATCHER_CRITERIA_MISC3)
		dst_mask->misc3 = src_mask->misc3;

	if (match_criteria & DR_MATCHER_CRITERIA_MISC4)
		dst_mask->misc4 = src_mask->misc4;
}

static void dr_matcher_destroy_definer_objs(struct mlx5dr_domain *dmn,
					    struct mlx5dr_ste_build *sb,
					    u8 idx)
{
	int i;

	for (i = 0; i < idx; i++) {
		mlx5dr_cmd_destroy_definer(dmn->mdev, sb[i].definer_id);
		memset(&sb[i], 0, sizeof(sb[i]));
	}
}

static int dr_matcher_create_definer_objs(struct mlx5dr_domain *dmn,
					  struct mlx5dr_ste_build *sb,
					  u8 idx)
{
	u32 definer_id;
	int i, ret;

	for (i = 0; i < idx; i++) {
		ret = mlx5dr_cmd_create_definer(dmn->mdev,
						sb[i].format_id,
						sb[i].match,
						&definer_id);
		if (ret)
			goto cleanup;

		/* The lu_type combines the definer and the entry type */
		sb[i].lu_type |= definer_id;
		sb[i].htbl_type = DR_STE_HTBL_TYPE_MATCH;
		sb[i].definer_id = definer_id;
	}

	return 0;

cleanup:
	dr_matcher_destroy_definer_objs(dmn, sb, i);
	return ret;
}

static void dr_matcher_clear_definers_builders(struct mlx5dr_ste_build *sb, u8 idx)
{
	u8 i;

	for (i = 0; i < idx; i++)
		memset(&sb[i], 0, sizeof(*sb));
}

static void dr_matcher_adjust_definer_ipv(struct mlx5dr_match_param *mask, u8 ipv)
{
	if (mask->outer.ip_version == 0xf)
		mask->outer.ip_version = ipv;
	if (mask->inner.ip_version == 0xf)
		mask->inner.ip_version = ipv;
}

static int dr_matcher_set_definer_builders(struct mlx5dr_matcher *matcher,
					   struct mlx5dr_matcher_rx_tx *nic_matcher,
					   struct mlx5dr_ste_build *sb,
					   u8 *idx)
{
	struct mlx5dr_domain_rx_tx *nic_dmn = nic_matcher->nic_tbl->nic_dmn;
	bool rx = nic_dmn->type == DR_DOMAIN_NIC_TYPE_RX;
	struct mlx5dr_domain *dmn = matcher->tbl->dmn;
	struct mlx5dr_cmd_caps *caps = &dmn->info.caps;
	struct mlx5dr_ste_ctx *ste_ctx = dmn->ste_ctx;
	struct mlx5dr_match_param mask = {};
	bool src_ipv6, dst_ipv6;
	u8 inner_ipv, outer_ipv;
	int ret;

	*idx = 0;
	outer_ipv = matcher->mask.outer.ip_version;
	inner_ipv = matcher->mask.inner.ip_version;

	src_ipv6 = dr_mask_is_src_addr_set(&matcher->mask.outer);
	dst_ipv6 = dr_mask_is_dst_addr_set(&matcher->mask.outer);

	if (caps->definer_format_sup & (1 << DR_MATCHER_DEFINER_0)) {
		dr_matcher_copy_mask(&mask, &matcher->mask, matcher->match_criteria);
		dr_matcher_adjust_definer_ipv(&mask, 4);
		ret = mlx5dr_ste_build_def0(ste_ctx, &sb[(*idx)++], &mask, caps, false, rx);
		if (!ret && dr_matcher_is_mask_consumed(&mask))
			goto done;

		memset(sb, 0, sizeof(*sb));
		*idx = 0;
	}

	if (caps->definer_format_sup & (1 << DR_MATCHER_DEFINER_22)) {
		dr_matcher_copy_mask(&mask, &matcher->mask, matcher->match_criteria);
		dr_matcher_adjust_definer_ipv(&mask, 4);
		ret = mlx5dr_ste_build_def22(ste_ctx, &sb[(*idx)++], &mask, false, rx);
		if (!ret && dr_matcher_is_mask_consumed(&mask))
			goto done;

		memset(sb, 0, sizeof(*sb));
		*idx = 0;
	}

	if (caps->definer_format_sup & (1 << DR_MATCHER_DEFINER_24)) {
		dr_matcher_copy_mask(&mask, &matcher->mask, matcher->match_criteria);
		dr_matcher_adjust_definer_ipv(&mask, 4);
		ret = mlx5dr_ste_build_def24(ste_ctx, &sb[(*idx)++], &mask, false, rx);
		if (!ret && dr_matcher_is_mask_consumed(&mask))
			goto done;

		memset(sb, 0, sizeof(*sb));
		*idx = 0;
	}

	if (caps->definer_format_sup & (1 << DR_MATCHER_DEFINER_25)) {
		dr_matcher_copy_mask(&mask, &matcher->mask, matcher->match_criteria);
		dr_matcher_adjust_definer_ipv(&mask, 4);
		ret = mlx5dr_ste_build_def25(ste_ctx, &sb[(*idx)++], &mask, false, rx);
		if (!ret && dr_matcher_is_mask_consumed(&mask))
			goto done;

		memset(sb, 0, sizeof(*sb));
		*idx = 0;
	}

	if (src_ipv6 &&
	    (caps->definer_format_sup & (1 << DR_MATCHER_DEFINER_6)) &&
	    (caps->definer_format_sup & (1 << DR_MATCHER_DEFINER_26))) {
		dr_matcher_copy_mask(&mask, &matcher->mask, matcher->match_criteria);
		dr_matcher_adjust_definer_ipv(&mask, 6);
		ret = mlx5dr_ste_build_def26(ste_ctx, &sb[(*idx)++],
					     &mask, false, rx);
		if (!ret && dst_ipv6)
			ret = mlx5dr_ste_build_def6(ste_ctx, &sb[(*idx)++],
						    &mask, false, rx);

		if (!ret && dr_matcher_is_mask_consumed(&mask))
			goto done;

		memset(&sb[0], 0, sizeof(*sb));
		memset(&sb[1], 0, sizeof(*sb));
		*idx = 0;
	}

	return -EOPNOTSUPP;

done:
	return 0;
}

static int dr_matcher_set_large_ste_builders(struct mlx5dr_matcher *matcher,
					     struct mlx5dr_matcher_rx_tx *nic_matcher,
					     u8 outer_ipv,
					     u8 inner_ipv)
{
	struct mlx5dr_ste_build *sb = nic_matcher->ste_builder_arr[outer_ipv][inner_ipv];
	struct mlx5dr_domain *dmn = matcher->tbl->dmn;
	int ret;
	u8 idx;

	/* Disabling match definers */
	return -EOPNOTSUPP;

	if (dmn->info.caps.sw_format_ver != MLX5_HW_CONNECTX_6DX ||
	    !dmn->info.caps.definer_format_sup)
		return -EOPNOTSUPP;

	ret = dr_matcher_set_definer_builders(matcher, nic_matcher, sb, &idx);
	if (ret)
		return ret;

	ret = dr_matcher_create_definer_objs(dmn, sb, idx);
	if (ret)
		goto clear_definers_builders;

	nic_matcher->num_of_builders_arr[outer_ipv][inner_ipv] = idx;
	nic_matcher->num_of_builders = idx;
	nic_matcher->ste_builder = sb;

	return 0;

clear_definers_builders:
	dr_matcher_clear_definers_builders(sb, idx);
	return ret;
}

static void dr_matcher_clear_ste_builders(struct mlx5dr_matcher *matcher,
					  struct mlx5dr_matcher_rx_tx *nic_matcher)
{
	struct mlx5dr_ste_build *sb;
	u8 num_of_builders;
	u32 i, j;

	for (i = 0; i < DR_RULE_IPV_MAX; i++) {
		for (j = 0; j < DR_RULE_IPV_MAX; j++) {
			sb = nic_matcher->ste_builder_arr[i][j];
			num_of_builders = nic_matcher->num_of_builders_arr[i][j];
			if (sb->htbl_type == DR_STE_HTBL_TYPE_MATCH)
				dr_matcher_destroy_definer_objs(matcher->tbl->dmn,
								sb,
								num_of_builders);
		}
	}
}

int mlx5dr_matcher_select_builders(struct mlx5dr_matcher *matcher,
				   struct mlx5dr_matcher_rx_tx *nic_matcher,
				   enum mlx5dr_ipv outer_ipv,
				   enum mlx5dr_ipv inner_ipv)
{
	nic_matcher->ste_builder =
		nic_matcher->ste_builder_arr[outer_ipv][inner_ipv];
	nic_matcher->num_of_builders =
		nic_matcher->num_of_builders_arr[outer_ipv][inner_ipv];

	if (!nic_matcher->num_of_builders) {
		mlx5dr_dbg(matcher->tbl->dmn,
			   "Rule not supported on this matcher due to IP related fields\n");
		return -EINVAL;
	}

	return 0;
}

static int dr_matcher_set_ste_builders(struct mlx5dr_matcher *matcher,
				       struct mlx5dr_matcher_rx_tx *nic_matcher,
				       enum mlx5dr_ipv outer_ipv,
				       enum mlx5dr_ipv inner_ipv)
{
	struct mlx5dr_domain_rx_tx *nic_dmn = nic_matcher->nic_tbl->nic_dmn;
	struct mlx5dr_domain *dmn = matcher->tbl->dmn;
	struct mlx5dr_ste_ctx *ste_ctx = dmn->ste_ctx;
	struct mlx5dr_match_param mask = {};
	bool allow_empty_match = false;
	struct mlx5dr_ste_build *sb;
	bool inner, rx;
	int idx = 0;
	int ret;

	sb = nic_matcher->ste_builder_arr[outer_ipv][inner_ipv];
	rx = nic_dmn->type == DR_DOMAIN_NIC_TYPE_RX;

	ret = mlx5dr_ste_build_pre_check(dmn, matcher->match_criteria,
					 &matcher->mask, NULL);
	if (ret)
		return ret;

	/* Use a large definers for matching if possible */
	ret = dr_matcher_set_large_ste_builders(matcher, nic_matcher, outer_ipv, inner_ipv);
	if (!ret)
		return 0;

	/* Create a temporary mask to track and clear used mask fields */
	dr_matcher_copy_mask(&mask, &matcher->mask, matcher->match_criteria);

	/* Optimize RX pipe by reducing source port match, since
	 * the FDB RX part is connected only to the wire.
	 */
	if (dmn->type == MLX5DR_DOMAIN_TYPE_FDB &&
	    rx && mask.misc.source_port) {
		mask.misc.source_port = 0;
		mask.misc.source_eswitch_owner_vhca_id = 0;
		allow_empty_match = true;
	}

	/* Outer */
	if (matcher->match_criteria & (DR_MATCHER_CRITERIA_OUTER |
				       DR_MATCHER_CRITERIA_MISC |
				       DR_MATCHER_CRITERIA_MISC2 |
				       DR_MATCHER_CRITERIA_MISC3)) {
		inner = false;

		if (dr_mask_is_wqe_metadata_set(&mask.misc2))
			mlx5dr_ste_build_general_purpose(ste_ctx, &sb[idx++],
							 &mask, inner, rx);

		if (dr_mask_is_reg_c_0_3_set(&mask.misc2))
			mlx5dr_ste_build_register_0(ste_ctx, &sb[idx++],
						    &mask, inner, rx);

		if (dr_mask_is_reg_c_4_7_set(&mask.misc2))
			mlx5dr_ste_build_register_1(ste_ctx, &sb[idx++],
						    &mask, inner, rx);

		if (dr_mask_is_gvmi_or_qpn_set(&mask.misc) &&
		    (dmn->type == MLX5DR_DOMAIN_TYPE_FDB ||
		     dmn->type == MLX5DR_DOMAIN_TYPE_NIC_RX)) {
			mlx5dr_ste_build_src_gvmi_qpn(ste_ctx, &sb[idx++],
						      &mask, dmn, inner, rx);
		}

		if (dr_mask_is_smac_set(&mask.outer) &&
		    dr_mask_is_dmac_set(&mask.outer)) {
			mlx5dr_ste_build_eth_l2_src_dst(ste_ctx, &sb[idx++],
							&mask, inner, rx);
		}

		if (dr_mask_is_smac_set(&mask.outer))
			mlx5dr_ste_build_eth_l2_src(ste_ctx, &sb[idx++],
						    &mask, inner, rx);

		if (DR_MASK_IS_L2_DST(mask.outer, mask.misc, outer))
			mlx5dr_ste_build_eth_l2_dst(ste_ctx, &sb[idx++],
						    &mask, inner, rx);

		if (outer_ipv == DR_RULE_IPV6) {
			if (dr_mask_is_dst_addr_set(&mask.outer))
				mlx5dr_ste_build_eth_l3_ipv6_dst(ste_ctx, &sb[idx++],
								 &mask, inner, rx);

			if (dr_mask_is_src_addr_set(&mask.outer))
				mlx5dr_ste_build_eth_l3_ipv6_src(ste_ctx, &sb[idx++],
								 &mask, inner, rx);

			if (DR_MASK_IS_ETH_L4_SET(mask.outer, mask.misc, outer))
				mlx5dr_ste_build_eth_ipv6_l3_l4(ste_ctx, &sb[idx++],
								&mask, inner, rx);
		} else {
			if (dr_mask_is_ipv4_5_tuple_set(&mask.outer))
				mlx5dr_ste_build_eth_l3_ipv4_5_tuple(ste_ctx, &sb[idx++],
								     &mask, inner, rx);

			if (dr_mask_is_ttl_set(&mask.outer))
				mlx5dr_ste_build_eth_l3_ipv4_misc(ste_ctx, &sb[idx++],
								  &mask, inner, rx);
		}

		if (dr_mask_is_tnl_vxlan_gpe(&mask, dmn))
			mlx5dr_ste_build_tnl_vxlan_gpe(ste_ctx, &sb[idx++],
						       &mask, inner, rx);
		else if (dr_mask_is_tnl_geneve(&mask, dmn)) {
			mlx5dr_ste_build_tnl_geneve(ste_ctx, &sb[idx++],
						    &mask, inner, rx);
			if (dr_mask_is_tnl_geneve_tlv_option(&mask.misc3))
				mlx5dr_ste_build_tnl_geneve_tlv_option(ste_ctx, &sb[idx++],
								       &mask, &dmn->info.caps,
								       inner, rx);
		}
		if (DR_MASK_IS_ETH_L4_MISC_SET(mask.misc3, outer))
			mlx5dr_ste_build_eth_l4_misc(ste_ctx, &sb[idx++],
						     &mask, inner, rx);

		if (DR_MASK_IS_FIRST_MPLS_SET(mask.misc2, outer))
			mlx5dr_ste_build_mpls(ste_ctx, &sb[idx++],
					      &mask, inner, rx);

		if (dr_mask_is_tnl_mpls_over_gre(&mask, dmn))
			mlx5dr_ste_build_tnl_mpls_over_gre(ste_ctx, &sb[idx++],
							   &mask, &dmn->info.caps,
							   inner, rx);

		else if (dr_mask_is_tnl_mpls_over_udp(&mask, dmn))
			mlx5dr_ste_build_tnl_mpls_over_udp(ste_ctx, &sb[idx++],
							   &mask, &dmn->info.caps,
							   inner, rx);

		if (dr_mask_is_icmp(&mask, dmn))
			mlx5dr_ste_build_icmp(ste_ctx, &sb[idx++],
					      &mask, &dmn->info.caps,
					      inner, rx);

		if (dr_mask_is_tnl_gre_set(&mask.misc))
			mlx5dr_ste_build_tnl_gre(ste_ctx, &sb[idx++],
						 &mask, inner, rx);
	}

	/* Inner */
	if (matcher->match_criteria & (DR_MATCHER_CRITERIA_INNER |
				       DR_MATCHER_CRITERIA_MISC |
				       DR_MATCHER_CRITERIA_MISC2 |
				       DR_MATCHER_CRITERIA_MISC3)) {
		inner = true;

		if (dr_mask_is_eth_l2_tnl_set(&mask.misc))
			mlx5dr_ste_build_eth_l2_tnl(ste_ctx, &sb[idx++],
						    &mask, inner, rx);

		if (dr_mask_is_smac_set(&mask.inner) &&
		    dr_mask_is_dmac_set(&mask.inner)) {
			mlx5dr_ste_build_eth_l2_src_dst(ste_ctx, &sb[idx++],
							&mask, inner, rx);
		}

		if (dr_mask_is_smac_set(&mask.inner))
			mlx5dr_ste_build_eth_l2_src(ste_ctx, &sb[idx++],
						    &mask, inner, rx);

		if (DR_MASK_IS_L2_DST(mask.inner, mask.misc, inner))
			mlx5dr_ste_build_eth_l2_dst(ste_ctx, &sb[idx++],
						    &mask, inner, rx);

		if (inner_ipv == DR_RULE_IPV6) {
			if (dr_mask_is_dst_addr_set(&mask.inner))
				mlx5dr_ste_build_eth_l3_ipv6_dst(ste_ctx, &sb[idx++],
								 &mask, inner, rx);

			if (dr_mask_is_src_addr_set(&mask.inner))
				mlx5dr_ste_build_eth_l3_ipv6_src(ste_ctx, &sb[idx++],
								 &mask, inner, rx);

			if (DR_MASK_IS_ETH_L4_SET(mask.inner, mask.misc, inner))
				mlx5dr_ste_build_eth_ipv6_l3_l4(ste_ctx, &sb[idx++],
								&mask, inner, rx);
		} else {
			if (dr_mask_is_ipv4_5_tuple_set(&mask.inner))
				mlx5dr_ste_build_eth_l3_ipv4_5_tuple(ste_ctx, &sb[idx++],
								     &mask, inner, rx);

			if (dr_mask_is_ttl_set(&mask.inner))
				mlx5dr_ste_build_eth_l3_ipv4_misc(ste_ctx, &sb[idx++],
								  &mask, inner, rx);
		}

		if (DR_MASK_IS_ETH_L4_MISC_SET(mask.misc3, inner))
			mlx5dr_ste_build_eth_l4_misc(ste_ctx, &sb[idx++],
						     &mask, inner, rx);

		if (DR_MASK_IS_FIRST_MPLS_SET(mask.misc2, inner))
			mlx5dr_ste_build_mpls(ste_ctx, &sb[idx++],
					      &mask, inner, rx);

		if (dr_mask_is_tnl_mpls_over_gre(&mask, dmn))
			mlx5dr_ste_build_tnl_mpls_over_gre(ste_ctx, &sb[idx++],
							   &mask, &dmn->info.caps,
							   inner, rx);

		else if (dr_mask_is_tnl_mpls_over_udp(&mask, dmn))
			mlx5dr_ste_build_tnl_mpls_over_udp(ste_ctx, &sb[idx++],
							   &mask, &dmn->info.caps,
							   inner, rx);

	}

	if (matcher->match_criteria & DR_MATCHER_CRITERIA_MISC4) {
		if (dr_mask_is_flex_parser_0_3_set(&mask.misc4))
			mlx5dr_ste_build_flex_parser_0(ste_ctx, &sb[idx++],
						       &mask, false, rx);

		if (dr_mask_is_flex_parser_4_7_set(&mask.misc4))
			mlx5dr_ste_build_flex_parser_1(ste_ctx, &sb[idx++],
						       &mask, false, rx);
	}

	/* Empty matcher, takes all */
	if ((!idx && allow_empty_match) ||
	    matcher->match_criteria == DR_MATCHER_CRITERIA_EMPTY)
		mlx5dr_ste_build_empty_always_hit(&sb[idx++], rx);

	if (idx == 0) {
		mlx5dr_err(dmn, "Cannot generate any valid rules from mask\n");
		return -EINVAL;
	}

	/* Check that all mask fields were consumed */
	if (!dr_matcher_is_mask_consumed(&mask)) {
		mlx5dr_dbg(dmn, "Mask contains unsupported parameters\n");
		return -EOPNOTSUPP;
	}

	nic_matcher->ste_builder = sb;
	nic_matcher->num_of_builders_arr[outer_ipv][inner_ipv] = idx;

	return 0;
}

static int dr_matcher_connect(struct mlx5dr_domain *dmn,
			      struct mlx5dr_matcher_rx_tx *curr_nic_matcher,
			      struct mlx5dr_matcher_rx_tx *next_nic_matcher,
			      struct mlx5dr_matcher_rx_tx *prev_nic_matcher)
{
	struct mlx5dr_table_rx_tx *nic_tbl = curr_nic_matcher->nic_tbl;
	struct mlx5dr_domain_rx_tx *nic_dmn = nic_tbl->nic_dmn;
	struct mlx5dr_htbl_connect_info info;
	struct mlx5dr_ste_htbl *prev_htbl;
	int ret;

	/* Connect end anchor hash table to next_htbl or to the default address */
	if (next_nic_matcher) {
		info.type = CONNECT_HIT;
		info.hit_next_htbl = next_nic_matcher->s_htbl;
	} else {
		info.type = CONNECT_MISS;
		info.miss_icm_addr = nic_tbl->default_icm_addr;
	}
	ret = mlx5dr_ste_htbl_init_and_postsend(dmn, nic_dmn,
						curr_nic_matcher->e_anchor,
						&info, info.type == CONNECT_HIT);
	if (ret)
		return ret;

	/* Connect start hash table to end anchor */
	info.type = CONNECT_MISS;
	info.miss_icm_addr = curr_nic_matcher->e_anchor->chunk->icm_addr;
	ret = mlx5dr_ste_htbl_init_and_postsend(dmn, nic_dmn,
						curr_nic_matcher->s_htbl,
						&info, false);
	if (ret)
		return ret;

	/* Connect previous hash table to matcher start hash table */
	if (prev_nic_matcher)
		prev_htbl = prev_nic_matcher->e_anchor;
	else
		prev_htbl = nic_tbl->s_anchor;

	info.type = CONNECT_HIT;
	info.hit_next_htbl = curr_nic_matcher->s_htbl;
	ret = mlx5dr_ste_htbl_init_and_postsend(dmn, nic_dmn, prev_htbl,
						&info, true);
	if (ret)
		return ret;

	/* Update the pointing ste and next hash table */
	curr_nic_matcher->s_htbl->pointing_ste = prev_htbl->ste_arr;
	prev_htbl->ste_arr[0].next_htbl = curr_nic_matcher->s_htbl;

	if (next_nic_matcher) {
		next_nic_matcher->s_htbl->pointing_ste = curr_nic_matcher->e_anchor->ste_arr;
		curr_nic_matcher->e_anchor->ste_arr[0].next_htbl = next_nic_matcher->s_htbl;
	}

	return 0;
}

static int dr_matcher_add_to_tbl(struct mlx5dr_matcher *matcher)
{
	struct mlx5dr_matcher *next_matcher, *prev_matcher, *tmp_matcher;
	struct mlx5dr_table *tbl = matcher->tbl;
	struct mlx5dr_domain *dmn = tbl->dmn;
	bool first = true;
	int ret;

	next_matcher = NULL;
	list_for_each_entry(tmp_matcher, &tbl->matcher_list, matcher_list) {
		if (tmp_matcher->prio >= matcher->prio) {
			next_matcher = tmp_matcher;
			break;
		}
		first = false;
	}

	prev_matcher = NULL;
	if (next_matcher && !first)
		prev_matcher = list_prev_entry(next_matcher, matcher_list);
	else if (!first)
		prev_matcher = list_last_entry(&tbl->matcher_list,
					       struct mlx5dr_matcher,
					       matcher_list);

	if (dmn->type == MLX5DR_DOMAIN_TYPE_FDB ||
	    dmn->type == MLX5DR_DOMAIN_TYPE_NIC_RX) {
		ret = dr_matcher_connect(dmn, &matcher->rx,
					 next_matcher ? &next_matcher->rx : NULL,
					 prev_matcher ?	&prev_matcher->rx : NULL);
		if (ret)
			return ret;
	}

	if (dmn->type == MLX5DR_DOMAIN_TYPE_FDB ||
	    dmn->type == MLX5DR_DOMAIN_TYPE_NIC_TX) {
		ret = dr_matcher_connect(dmn, &matcher->tx,
					 next_matcher ? &next_matcher->tx : NULL,
					 prev_matcher ?	&prev_matcher->tx : NULL);
		if (ret)
			return ret;
	}

	mutex_lock(&tbl->dmn->dbg_mutex);

	if (prev_matcher)
		list_add(&matcher->matcher_list, &prev_matcher->matcher_list);
	else if (next_matcher)
		list_add_tail(&matcher->matcher_list,
			      &next_matcher->matcher_list);
	else
		list_add(&matcher->matcher_list, &tbl->matcher_list);

	mutex_unlock(&tbl->dmn->dbg_mutex);

	return 0;
}

static void dr_matcher_uninit_nic(struct mlx5dr_matcher *matcher,
				  struct mlx5dr_matcher_rx_tx *nic_matcher)
{
	dr_matcher_clear_ste_builders(matcher, nic_matcher);
	mlx5dr_htbl_put(nic_matcher->s_htbl);
	mlx5dr_htbl_put(nic_matcher->e_anchor);
}

static void dr_matcher_uninit_fdb(struct mlx5dr_matcher *matcher)
{
	dr_matcher_uninit_nic(matcher, &matcher->rx);
	dr_matcher_uninit_nic(matcher, &matcher->tx);
}

static void dr_matcher_uninit(struct mlx5dr_matcher *matcher)
{
	struct mlx5dr_domain *dmn = matcher->tbl->dmn;

	switch (dmn->type) {
	case MLX5DR_DOMAIN_TYPE_NIC_RX:
		dr_matcher_uninit_nic(matcher, &matcher->rx);
		break;
	case MLX5DR_DOMAIN_TYPE_NIC_TX:
		dr_matcher_uninit_nic(matcher, &matcher->tx);
		break;
	case MLX5DR_DOMAIN_TYPE_FDB:
		dr_matcher_uninit_fdb(matcher);
		break;
	default:
		WARN_ON(true);
		break;
	}
}

static int dr_matcher_get_mask_ip_versions(struct mlx5dr_matcher *matcher,
					   bool *outer_ipv4, bool *outer_ipv6,
					   bool *inner_ipv4, bool *inner_ipv6)
{
	bool outer_ipv6_mask_present = false;
	bool inner_ipv6_mask_present = false;
	u8 outer_ipv = 0, inner_ipv = 0;

	if (matcher->match_criteria & DR_MATCHER_CRITERIA_OUTER) {
		outer_ipv = matcher->mask.outer.ip_version;
		outer_ipv6_mask_present =
			dr_mask_is_ipv6_only_match_set(&matcher->mask.outer);
	}
	if (matcher->match_criteria & DR_MATCHER_CRITERIA_INNER) {
		inner_ipv = matcher->mask.inner.ip_version;
		inner_ipv6_mask_present =
			dr_mask_is_ipv6_only_match_set(&matcher->mask.inner);
	}

	if ((outer_ipv == 4 && outer_ipv6_mask_present) ||
	    (inner_ipv == 4 && inner_ipv6_mask_present))
		return -EINVAL;

	if (outer_ipv == 0xf) {
		*outer_ipv4 = true;
		*outer_ipv6 = true;
	} else if (outer_ipv == 6 || outer_ipv6_mask_present) {
		*outer_ipv4 = false;
		*outer_ipv6 = true;
	} else {
		*outer_ipv4 = true;
		*outer_ipv6 = false;
	}

	if (inner_ipv == 0xf) {
		*inner_ipv4 = true;
		*inner_ipv6 = true;
	} else if (inner_ipv == 6 || inner_ipv6_mask_present) {
		*inner_ipv4 = false;
		*inner_ipv6 = true;
	} else {
		*inner_ipv4 = true;
		*inner_ipv6 = false;
	}

	return 0;
}

static int dr_matcher_set_all_ste_builders(struct mlx5dr_matcher *matcher,
					   struct mlx5dr_matcher_rx_tx *nic_matcher)
{
	bool outer_ipv6, outer_ipv4, inner_ipv6, inner_ipv4;
	struct mlx5dr_domain *dmn = matcher->tbl->dmn;
	int ret;

	ret = dr_matcher_get_mask_ip_versions(matcher,
					      &outer_ipv4, &outer_ipv6,
					      &inner_ipv4, &inner_ipv6);
	if (ret) {
		mlx5dr_err(dmn, "Cannot generate IPv4/6 rules with given IP ver/addr mask\n");
		return ret;
	}

	if (outer_ipv4 && inner_ipv4)
		dr_matcher_set_ste_builders(matcher, nic_matcher, DR_RULE_IPV4, DR_RULE_IPV4);
	if (outer_ipv4 && inner_ipv6)
		dr_matcher_set_ste_builders(matcher, nic_matcher, DR_RULE_IPV4, DR_RULE_IPV6);
	if (outer_ipv6 && inner_ipv4)
		dr_matcher_set_ste_builders(matcher, nic_matcher, DR_RULE_IPV6, DR_RULE_IPV4);
	if (outer_ipv6 && inner_ipv6)
		dr_matcher_set_ste_builders(matcher, nic_matcher, DR_RULE_IPV6, DR_RULE_IPV6);

	if (!nic_matcher->ste_builder) {
		mlx5dr_err(dmn, "Cannot generate IPv4 or IPv6 rules with given mask\n");
		return -EINVAL;
	}

	return 0;
}

static int dr_matcher_init_nic(struct mlx5dr_matcher *matcher,
			       struct mlx5dr_matcher_rx_tx *nic_matcher)
{
	struct mlx5dr_domain *dmn = matcher->tbl->dmn;
	int ret;

	ret = dr_matcher_set_all_ste_builders(matcher, nic_matcher);
	if (ret)
		return ret;

	nic_matcher->e_anchor = mlx5dr_ste_htbl_alloc(dmn->ste_icm_pool,
						      DR_CHUNK_SIZE_1,
						      DR_STE_HTBL_TYPE_LEGACY,
						      MLX5DR_STE_LU_TYPE_DONT_CARE,
						      0);
	if (!nic_matcher->e_anchor) {
		ret = -ENOMEM;
		goto clear_ste_builders;
	}

	nic_matcher->s_htbl = mlx5dr_ste_htbl_alloc(dmn->ste_icm_pool,
						    DR_CHUNK_SIZE_1,
						    nic_matcher->ste_builder->htbl_type,
						    nic_matcher->ste_builder->lu_type,
						    nic_matcher->ste_builder->byte_mask);
	if (!nic_matcher->s_htbl) {
		ret = -ENOMEM;
		goto free_e_htbl;
	}

	/* make sure the tables exist while empty */
	mlx5dr_htbl_get(nic_matcher->s_htbl);
	mlx5dr_htbl_get(nic_matcher->e_anchor);

	return 0;

free_e_htbl:
	mlx5dr_ste_htbl_free(nic_matcher->e_anchor);
clear_ste_builders:
	dr_matcher_clear_ste_builders(matcher, nic_matcher);
	return ret;
}

static int dr_matcher_init_fdb(struct mlx5dr_matcher *matcher)
{
	int ret;

	ret = dr_matcher_init_nic(matcher, &matcher->rx);
	if (ret)
		return ret;

	ret = dr_matcher_init_nic(matcher, &matcher->tx);
	if (ret)
		goto uninit_nic_rx;

	return 0;

uninit_nic_rx:
	dr_matcher_uninit_nic(matcher, &matcher->rx);
	return ret;
}

static int dr_matcher_init(struct mlx5dr_matcher *matcher,
			   struct mlx5dr_match_parameters *mask)
{
	struct mlx5dr_table *tbl = matcher->tbl;
	struct mlx5dr_domain *dmn = tbl->dmn;
	int ret;

	if (matcher->match_criteria >= DR_MATCHER_CRITERIA_MAX) {
		mlx5dr_err(dmn, "Invalid match criteria attribute\n");
		return -EINVAL;
	}

	if (mask) {
		if (mask->match_sz > MLX5_ST_SZ_DW_MATCH_PARAM * 4) {
			mlx5dr_err(dmn, "Invalid match size attribute\n");
			return -EINVAL;
		}
		mlx5dr_ste_copy_param(matcher->match_criteria,
				      &matcher->mask, mask);
	}

	switch (dmn->type) {
	case MLX5DR_DOMAIN_TYPE_NIC_RX:
		matcher->rx.nic_tbl = &tbl->rx;
		ret = dr_matcher_init_nic(matcher, &matcher->rx);
		break;
	case MLX5DR_DOMAIN_TYPE_NIC_TX:
		matcher->tx.nic_tbl = &tbl->tx;
		ret = dr_matcher_init_nic(matcher, &matcher->tx);
		break;
	case MLX5DR_DOMAIN_TYPE_FDB:
		matcher->rx.nic_tbl = &tbl->rx;
		matcher->tx.nic_tbl = &tbl->tx;
		ret = dr_matcher_init_fdb(matcher);
		break;
	default:
		WARN_ON(true);
		return -EINVAL;
	}

	return ret;
}

struct mlx5dr_matcher *
mlx5dr_matcher_create(struct mlx5dr_table *tbl,
		      u32 priority,
		      u8 match_criteria_enable,
		      struct mlx5dr_match_parameters *mask)
{
	struct mlx5dr_matcher *matcher;
	int ret;

	refcount_inc(&tbl->refcount);

	matcher = kzalloc(sizeof(*matcher), GFP_KERNEL);
	if (!matcher)
		goto dec_ref;

	matcher->tbl = tbl;
	matcher->prio = priority;
	matcher->match_criteria = match_criteria_enable;
	refcount_set(&matcher->refcount, 1);
	INIT_LIST_HEAD(&matcher->matcher_list);
	INIT_LIST_HEAD(&matcher->rule_list);

	mlx5dr_domain_lock(tbl->dmn);

	ret = dr_matcher_init(matcher, mask);
	if (ret)
		goto free_matcher;

	ret = dr_matcher_add_to_tbl(matcher);
	if (ret)
		goto matcher_uninit;

	mlx5dr_domain_unlock(tbl->dmn);

	return matcher;

matcher_uninit:
	dr_matcher_uninit(matcher);
free_matcher:
	mlx5dr_domain_unlock(tbl->dmn);
	kfree(matcher);
dec_ref:
	refcount_dec(&tbl->refcount);
	return NULL;
}

static int dr_matcher_disconnect(struct mlx5dr_domain *dmn,
				 struct mlx5dr_table_rx_tx *nic_tbl,
				 struct mlx5dr_matcher_rx_tx *next_nic_matcher,
				 struct mlx5dr_matcher_rx_tx *prev_nic_matcher)
{
	struct mlx5dr_domain_rx_tx *nic_dmn = nic_tbl->nic_dmn;
	struct mlx5dr_htbl_connect_info info;
	struct mlx5dr_ste_htbl *prev_anchor;

	if (prev_nic_matcher)
		prev_anchor = prev_nic_matcher->e_anchor;
	else
		prev_anchor = nic_tbl->s_anchor;

	/* Connect previous anchor hash table to next matcher or to the default address */
	if (next_nic_matcher) {
		info.type = CONNECT_HIT;
		info.hit_next_htbl = next_nic_matcher->s_htbl;
		next_nic_matcher->s_htbl->pointing_ste = prev_anchor->ste_arr;
		prev_anchor->ste_arr[0].next_htbl = next_nic_matcher->s_htbl;
	} else {
		info.type = CONNECT_MISS;
		info.miss_icm_addr = nic_tbl->default_icm_addr;
		prev_anchor->ste_arr[0].next_htbl = NULL;
	}

	return mlx5dr_ste_htbl_init_and_postsend(dmn, nic_dmn, prev_anchor,
						 &info, true);
}

static int dr_matcher_remove_from_tbl(struct mlx5dr_matcher *matcher)
{
	struct mlx5dr_matcher *prev_matcher, *next_matcher;
	struct mlx5dr_table *tbl = matcher->tbl;
	struct mlx5dr_domain *dmn = tbl->dmn;
	int ret = 0;

	if (list_is_last(&matcher->matcher_list, &tbl->matcher_list))
		next_matcher = NULL;
	else
		next_matcher = list_next_entry(matcher, matcher_list);

	if (matcher->matcher_list.prev == &tbl->matcher_list)
		prev_matcher = NULL;
	else
		prev_matcher = list_prev_entry(matcher, matcher_list);

	if (dmn->type == MLX5DR_DOMAIN_TYPE_FDB ||
	    dmn->type == MLX5DR_DOMAIN_TYPE_NIC_RX) {
		ret = dr_matcher_disconnect(dmn, &tbl->rx,
					    next_matcher ? &next_matcher->rx : NULL,
					    prev_matcher ? &prev_matcher->rx : NULL);
		if (ret)
			return ret;
	}

	if (dmn->type == MLX5DR_DOMAIN_TYPE_FDB ||
	    dmn->type == MLX5DR_DOMAIN_TYPE_NIC_TX) {
		ret = dr_matcher_disconnect(dmn, &tbl->tx,
					    next_matcher ? &next_matcher->tx : NULL,
					    prev_matcher ? &prev_matcher->tx : NULL);
		if (ret)
			return ret;
	}

	list_del(&matcher->matcher_list);

	return 0;
}

int mlx5dr_matcher_destroy(struct mlx5dr_matcher *matcher)
{
	struct mlx5dr_table *tbl = matcher->tbl;

	if (refcount_read(&matcher->refcount) > 1)
		return -EBUSY;

	mlx5dr_domain_lock(tbl->dmn);
	mutex_lock(&tbl->dmn->dbg_mutex);
	dr_matcher_remove_from_tbl(matcher);
	mutex_unlock(&tbl->dmn->dbg_mutex);
	dr_matcher_uninit(matcher);
	refcount_dec(&matcher->tbl->refcount);

	mlx5dr_domain_unlock(tbl->dmn);
	kfree(matcher);

	return 0;
}