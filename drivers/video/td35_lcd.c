#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/fb.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/platform_data/video_s3c.h>
#include <video/samsung_fimd.h>
//MODULE_DEVICE_TABLE(platform, lcd_id_table);
#define LCDCON1 0x0
#define LCDCON2 0x4
#define LCDCON3 0x8
#define LCDCON4 0xc
#define LCDCON5 0x10
#define LCDSADDR1 0x14
#define LCDSADDR2 0x18
#define LCDSADDR3 0x1c
#define LCDTCONSEL 0x60
#define GPIO_BASE 0x56000000
#define GPIO_SIZE 0x1000
#define GPCCON 0X20
#define GPDCON 0X30
struct s3c_fb {
	spinlock_t		slock;
	struct device		*dev;
	struct clk		*bus_clk;
	struct clk		*lcd_clk;
	void __iomem		*regs;
	unsigned char		 enabled;
	bool			 output_on;
	int			 irq_no;
	unsigned long		 irq_flags;
	void __iomem		*gpio_regs;
	struct fb_info *FbInfo;
};

static u32 pseudo_palette[16];

static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int s3c_lcdfb_setcolreg(unsigned int regno, unsigned int red,
			     unsigned int green, unsigned int blue,
			     unsigned int transp, struct fb_info *info)
{
	unsigned int val;
	if (regno > 16)
		return 1;
	val  = chan_to_field(red,	&info->var.red);
	val |= chan_to_field(green, &info->var.green);
	val |= chan_to_field(blue,	&info->var.blue);
	pseudo_palette[regno] = val;
	return 0;
}


static struct fb_ops s3c_td35_ops = {
	.owner		= THIS_MODULE,
	.fb_setcolreg	= s3c_lcdfb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

void fill_td35_fb_parameter(struct fb_info *Fb_Info)
{
	strcpy(Fb_Info->fix.id,"s3c_td35");
	Fb_Info->fix.smem_len=320*240*2;
	Fb_Info->fix.type=FB_TYPE_PACKED_PIXELS;
	Fb_Info->fix.visual=FB_VISUAL_TRUECOLOR;
	Fb_Info->fix.line_length=240*2;
	Fb_Info->var.xres=240;
	Fb_Info->var.yres=320;
	Fb_Info->var.xres_virtual=240;
	Fb_Info->var.yres_virtual=320;
	Fb_Info->var.bits_per_pixel=16;
	Fb_Info->var.red.offset=11;
	Fb_Info->var.red.length=5;
	Fb_Info->var.green.offset=5;
	Fb_Info->var.green.length=6;
	Fb_Info->var.blue.offset=0;
	Fb_Info->var.blue.length=5;
	Fb_Info->var.transp.offset=0;
	Fb_Info->var.transp.length=0;
	Fb_Info->var.activate=FB_ACTIVATE_NOW;
	Fb_Info->fbops=&s3c_td35_ops;
	Fb_Info->pseudo_palette = pseudo_palette;
	Fb_Info->screen_size=320*240*2;
}

void s3c_init_lcd_controller_for_td35(struct s3c_fb *sfb)
{
	unsigned int SettingValue=0;
	// clkval =7,lcd clock=6.32Mhz, vm=0 ,TFT mode,TFT 16bpp,disable LCD first
	SettingValue= (7<<8) |(3<<5) | (12<<1);
	writel(SettingValue,sfb->regs+LCDCON1);

	SettingValue= (4<<24) | (319<<14) | (4<<6) | (4<<0);
	writel(SettingValue,sfb->regs+LCDCON2);

	//HSPW+HBPD+HFPD equal to 90 clock,TFT Horizontal Blank time equal to 90 clock
	SettingValue= (30<<19) | (239<<8) |(30<<0);
	writel(SettingValue,sfb->regs+LCDCON3);

	SettingValue= (30<<0);
	writel(SettingValue,sfb->regs+LCDCON4);

	SettingValue=(1<<11) |(1<<10) | (1<<9) | (1<<8)| 1;
	writel(SettingValue,sfb->regs+LCDCON5);

	sfb->FbInfo->screen_base = dma_alloc_writecombine(NULL, PAGE_ALIGN(sfb->FbInfo->fix.smem_len),&sfb->FbInfo->fix.smem_start, GFP_KERNEL);
	SettingValue=(((sfb->FbInfo->fix.smem_start)>>22)<<21) | (((sfb->FbInfo->fix.smem_start)>>1)&0x1FFFFF);
	writel(SettingValue,sfb->regs+LCDSADDR1);

	SettingValue=(sfb->FbInfo->fix.smem_start+sfb->FbInfo->var.yres * sfb->FbInfo->fix.line_length)>>1;
	writel(SettingValue,sfb->regs+LCDSADDR2);
	
	SettingValue=240&0x3ff;
	writel(SettingValue,sfb->regs+LCDSADDR3);

	SettingValue=0;
	writel(SettingValue,sfb->regs+LCDTCONSEL);

	//enable lcd controller
	SettingValue=readl(sfb->regs+LCDCON1) | 1;
	writel(SettingValue,sfb->regs+LCDCON1);

}

void s3c_init_lcd_config_gpio(struct s3c_fb *sfb){
	unsigned int SettingValue=0;
	sfb->gpio_regs = ioremap(GPIO_BASE, GPIO_SIZE);

	SettingValue=(2<<30)|(2<<28)|(2<<26)|(2<<24)|(2<<22)|(2<<20)|(2<<8)|(2<<2);
	writel(SettingValue,sfb->gpio_regs+GPCCON);

	SettingValue=(2<<30)|(2<<28)|(2<<26)|(2<<24)|(2<<22)|(2<<12)|(2<<10)|(2<<8)|(2<<6)|(2<<4);
	writel(SettingValue,sfb->gpio_regs+GPDCON);
}

static int td35_fb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct s3c_fb *sfb;
	struct resource *res;
	int ret=0;
	
	sfb = devm_kzalloc(dev, sizeof(struct s3c_fb), GFP_KERNEL);
	if (!sfb) {
		dev_err(dev, "no memory for framebuffers\n");
		return -ENOMEM;
	}
	memset(sfb,0,sizeof(struct s3c_fb));
	dev_dbg(dev, "allocate new framebuffer %p\n", sfb);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sfb->regs = ioremap(res->start, res->end-res->start);
	if (IS_ERR(sfb->regs)) {
		ret = PTR_ERR(sfb->regs);
		sfb->regs=NULL;
		goto err_devm_kzalloc;
	}
	
	platform_set_drvdata(pdev, sfb);
	sfb->FbInfo=framebuffer_alloc(0, sfb->dev);
	if (!sfb->FbInfo) {
		dev_err(sfb->dev, "failed to allocate framebuffer\n");
		goto err_alloc_resource;
	}
	memset(sfb->FbInfo,0,sizeof(struct fb_info));
	fill_td35_fb_parameter(sfb->FbInfo);
	s3c_init_lcd_config_gpio(sfb);
	s3c_init_lcd_controller_for_td35(sfb);
	register_framebuffer(sfb->FbInfo);
	sfb->bus_clk = clk_get(NULL, "lcd");
	if (IS_ERR(sfb->bus_clk)) {
		dev_err(dev, "failed to get bus clock\n");
		goto err_alloc_resource;
	}
	clk_enable(sfb->bus_clk);
	return 0;
err_alloc_resource:
	iounmap(sfb->regs);
err_devm_kzalloc:
	devm_kfree(dev,sfb);
	return ret;
}

static int td35_fb_remove(struct platform_device *pdev)
{
	struct s3c_fb *sfb = platform_get_drvdata(pdev);
	
	struct device *dev = &pdev->dev;
	if(sfb){
		clk_disable(sfb->bus_clk);
		clk_put(sfb->bus_clk);
		unregister_framebuffer(sfb->FbInfo);
		if(sfb->regs)
			iounmap(sfb->regs);
		if(sfb->FbInfo->screen_base)
			dma_free_writecombine(NULL, PAGE_ALIGN(sfb->FbInfo->fix.smem_len),
			      sfb->FbInfo->screen_base, sfb->FbInfo->fix.smem_start);
		if(sfb->gpio_regs)
			iounmap(sfb->gpio_regs);
		devm_kfree(dev,sfb);
	}
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id lcd_id_table[] = {
	{ .compatible = "s3c,td35" },
	{}
};
#endif

static struct platform_driver td35_lcd = {
	.probe		= td35_fb_probe,
	.remove		= td35_fb_remove,
	.driver		= {
		.name	= "td35",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(lcd_id_table),
	},
};

module_platform_driver(td35_lcd);
MODULE_DESCRIPTION("Samsung S3C SoC Framebuffer driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:s3c-td35");
