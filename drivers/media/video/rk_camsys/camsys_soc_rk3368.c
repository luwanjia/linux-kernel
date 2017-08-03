#ifdef CONFIG_ARM64
#include "camsys_soc_priv.h"
#include "camsys_soc_rk3368.h"


struct mipiphy_hsfreqrange_s {
	unsigned int range_l;
	unsigned int range_h;
	unsigned char cfg_bit;
};

static struct mipiphy_hsfreqrange_s mipiphy_hsfreqrange[] = {
	{80, 90, 0x00},
	{90, 100, 0x10},
	{100, 110, 0x20},
	{110, 130, 0x01},
	{130, 140, 0x11},
	{140, 150, 0x21},
	{150, 170, 0x02},
	{170, 180, 0x12},
	{180, 200, 0x22},
	{200, 220, 0x03},
	{220, 240, 0x13},
	{240, 250, 0x23},
	{250, 270, 0x04},
	{270, 300, 0x14},
	{300, 330, 0x05},
	{330, 360, 0x15},
	{360, 400, 0x25},
	{400, 450, 0x06},
	{450, 500, 0x16},
	{500, 550, 0x07},
	{550, 600, 0x17},
	{600, 650, 0x08},
	{650, 700, 0x18},
	{700, 750, 0x09},
	{750, 800, 0x19},
	{800, 850, 0x29},
	{850, 900, 0x39},
	{900, 950, 0x0a},
	{950, 1000, 0x1a},
	{1000, 1050, 0x2a},
	{1100, 1150, 0x3a},
	{1150, 1200, 0x0b},
	{1200, 1250, 0x1b},
	{1250, 1300, 0x2b},
	{1300, 1350, 0x0c},
	{1350, 1400, 0x1c},
	{1400, 1450, 0x2c},
	{1450, 1500, 0x3c}

};

#if 0
static int camsys_rk3368_mipiphy_wr_reg
(unsigned long phy_virt, unsigned char addr, unsigned char data)
{
	/*TESTEN =1,TESTDIN=addr */
	write_csihost_reg(CSIHOST_PHY_TEST_CTRL1, (0x00010000 | addr));
	/*TESTCLK=0 */
	write_csihost_reg(CSIHOST_PHY_TEST_CTRL0, 0x00000000);
	udelay(10);
	/*TESTEN =0,TESTDIN=data */
	write_csihost_reg(CSIHOST_PHY_TEST_CTRL1, (0x00000000 | data));
	/*TESTCLK=1 */
	write_csihost_reg(CSIHOST_PHY_TEST_CTRL0, 0x00000002);
	udelay(10);

	return 0;
}

static int camsys_rk3368_mipiphy_rd_reg
(unsigned long phy_virt, unsigned char addr)
{
	return (read_csihost_reg(((CSIHOST_PHY_TEST_CTRL1)&0xff00))>>8);
}

static int camsys_rk3368_csiphy_wr_reg
(unsigned long csiphy_virt, unsigned char addr, unsigned char data)
{
	write_csiphy_reg(addr, data);
	return 0;
}

static int camsys_rk3368_csiphy_rd_reg
(unsigned long csiphy_virt, unsigned char addr)
{
	return read_csiphy_reg(addr);
}
#endif
static int camsys_rk3368_mipihpy_cfg(camsys_mipiphy_soc_para_t *para)
{
	unsigned char hsfreqrange = 0xff, i;
	struct mipiphy_hsfreqrange_s *hsfreqrange_p;
	unsigned long phy_virt, phy_index;
	unsigned long base;
	unsigned long csiphy_virt;

	phy_index = para->phy->phy_index;
	if (para->camsys_dev->mipiphy[phy_index].reg != NULL) {
		phy_virt = para->camsys_dev->mipiphy[phy_index].reg->vir_base;
	} else {
		phy_virt = 0x00;
	}
	if (para->camsys_dev->csiphy_reg != NULL) {
		csiphy_virt =
		(unsigned long)para->camsys_dev->csiphy_reg->vir_base;
	} else {
		csiphy_virt = 0x00;
	}
	if ((para->phy->bit_rate == 0) ||
		(para->phy->data_en_bit == 0)) {
		if (para->phy->phy_index == 0) {
			base =
			(unsigned long)
			para->camsys_dev->devmems.registermem->vir_base;
			*((unsigned int *)
				(base + (MRV_MIPI_BASE + MRV_MIPI_CTRL)))
				&= ~(0x0f << 8);
			camsys_trace(1, "mipi phy 0 standby!");
		}

		return 0;
	}

	hsfreqrange_p = mipiphy_hsfreqrange;
	for (i = 0;
		i < (sizeof(mipiphy_hsfreqrange)/
			sizeof(struct mipiphy_hsfreqrange_s));
		i++) {

		if ((para->phy->bit_rate > hsfreqrange_p->range_l) &&
			(para->phy->bit_rate <= hsfreqrange_p->range_h)) {
			hsfreqrange = hsfreqrange_p->cfg_bit;
			break;
		}
		hsfreqrange_p++;
	}

	if (hsfreqrange == 0xff) {
		camsys_err("mipi phy config bitrate %d Mbps isn't supported!",
			para->phy->bit_rate);
		hsfreqrange = 0x00;
	}

	if (para->phy->phy_index == 0) {
	/* isp select */
	write_grf_reg(GRF_SOC_CON6_OFFSET, ISP_MIPI_CSI_HOST_SEL_OFFSET_MASK
				| (1 << ISP_MIPI_CSI_HOST_SEL_OFFSET_BIT));

	/* phy start */
	write_csiphy_reg((MIPI_CSI_DPHY_LANEX_THS_SETTLE_OFFSET + 0x100),
		hsfreqrange |
		(read_csiphy_reg(MIPI_CSI_DPHY_LANEX_THS_SETTLE_OFFSET
		+ 0x100) & (~0xf)));

	if (para->phy->data_en_bit > 0x00) {
		write_csiphy_reg((MIPI_CSI_DPHY_LANEX_THS_SETTLE_OFFSET
			+ 0x180), hsfreqrange |
			(read_csiphy_reg(MIPI_CSI_DPHY_LANEX_THS_SETTLE_OFFSET
			+ 0x180) & (~0xf)));
	}
	if (para->phy->data_en_bit > 0x02) {
		write_csiphy_reg(MIPI_CSI_DPHY_LANEX_THS_SETTLE_OFFSET
			+ 0x200, hsfreqrange |
			(read_csiphy_reg(MIPI_CSI_DPHY_LANEX_THS_SETTLE_OFFSET
			+ 0x200) & (~0xf)));
	}
	if (para->phy->data_en_bit > 0x04) {
		write_csiphy_reg(MIPI_CSI_DPHY_LANEX_THS_SETTLE_OFFSET
			+ 0x280, hsfreqrange |
			(read_csiphy_reg(MIPI_CSI_DPHY_LANEX_THS_SETTLE_OFFSET
			+ 0x280) & (~0xf)));
		write_csiphy_reg(MIPI_CSI_DPHY_LANEX_THS_SETTLE_OFFSET
			+ 0x300, hsfreqrange |
			(read_csiphy_reg(MIPI_CSI_DPHY_LANEX_THS_SETTLE_OFFSET
			+ 0x300) & (~0xf)));
	}

	/*set data lane num and enable clock lane */
	write_csiphy_reg(0x00, ((para->phy->data_en_bit << 2)
					| (0x1 << 6) | 0x1));

	base = (unsigned long)para->camsys_dev->devmems.registermem->vir_base;
	*((unsigned int *)(base + (MRV_MIPI_BASE + MRV_MIPI_CTRL)))
							&= ~(0x0f << 8);
	} else {
		camsys_err("mipi phy index %d is invalidate!",
			para->phy->phy_index);
		goto fail;
	}

	camsys_trace(1, "mipi phy(%d) turn on(lane: 0x%x  bit_rate: %dMbps)",
		para->phy->phy_index,
		para->phy->data_en_bit, para->phy->bit_rate);

	return 0;

fail:
	return -1;
}

#define MRV_AFM_BASE		0x0000
#define VI_IRCL			0x0014
int camsys_rk3368_cfg(
camsys_dev_t *camsys_dev, camsys_soc_cfg_t cfg_cmd, void *cfg_para)
{
	unsigned int *para_int;

	switch (cfg_cmd) {
	case Clk_DriverStrength_Cfg: {
		para_int = (unsigned int *)cfg_para;
		__raw_writel((((*para_int) & 0x03) << 3) | (0x03 << 3),
				(void *)(camsys_dev->rk_grf_base + 0x204));
		/* set 0xffffffff to max all */
		break;
	}

	case Cif_IoDomain_Cfg: {
		para_int = (unsigned int *)cfg_para;
		if (*para_int < 28000000) {
			/* 1.8v IO */
			__raw_writel(((1 << 1) | (1 << (1 + 16))),
				(void *)(camsys_dev->rk_grf_base + 0x0900));
		} else {
			/* 3.3v IO */
			__raw_writel(((0 << 1) | (1 << (1 + 16))),
				(void *)(camsys_dev->rk_grf_base + 0x0900));
			}
		break;
	}

	case Mipi_Phy_Cfg: {
		camsys_rk3368_mipihpy_cfg
			((camsys_mipiphy_soc_para_t *)cfg_para);
		break;
	}

	case Isp_SoftRst: {/* ddl@rock-chips.com: v0.d.0 */
		unsigned long reset;

		reset = (unsigned long)cfg_para;

		if (reset == 1)
			__raw_writel(0x80, (void *)(camsys_dev->rk_isp_base +
			MRV_AFM_BASE + VI_IRCL));
		else
			__raw_writel(0x00, (void *)(camsys_dev->rk_isp_base +
			MRV_AFM_BASE + VI_IRCL));
			camsys_trace(2, "Isp self soft rst: %ld", reset);
			break;
		}

	default:
	{
		camsys_warn("cfg_cmd: 0x%x isn't support", cfg_cmd);
		break;
	}

	}

	return 0;
}
#endif /* CONFIG_ARM64 */
