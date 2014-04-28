#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT		12
#define PAGE_SIZE		(1UL << PAGE_SHIFT)
#define PAGE_MASK		(~(PAGE_SIZE-1))

//a framebuffer device structure;
typedef struct fbdev{
    int fb;
    unsigned long fb_mem_offset;
    unsigned long fb_mem;
    struct fb_fix_screeninfo fb_fix;
    struct fb_var_screeninfo fb_var;
    char dev[20];
} FBDEV, *PFBDEV;

int fb_open(PFBDEV pFbdev)
{
    pFbdev->fb = open(pFbdev->dev, O_RDWR);
    if(pFbdev->fb < 0)
    {
        printf("Error opening %s: %m. Check kernel config\n", pFbdev->dev);
        return -1;
    }
    if (-1 == ioctl(pFbdev->fb,FBIOGET_VSCREENINFO,&(pFbdev->fb_var)))
    {
        printf("ioctl FBIOGET_VSCREENINFO\n");
        return -1;
    }
	printf("xres: %d, yres: %d, xres_virtaul %d, yres_virtual %d, height %d, width %d\n", pFbdev->fb_var.xres, pFbdev->fb_var.yres, 
			pFbdev->fb_var.xres_virtual, pFbdev->fb_var.yres_virtual, pFbdev->fb_var.height, pFbdev->fb_var.width);
    if (-1 == ioctl(pFbdev->fb,FBIOGET_FSCREENINFO,&(pFbdev->fb_fix)))
    {
        printf("ioctl FBIOGET_FSCREENINFO\n");
        return -1;
    }
	printf("smem_start: %d, smem_len %d\n", pFbdev->fb_fix.smem_start, pFbdev->fb_fix.smem_len);
    //map physics address to virtual address
    pFbdev->fb_mem_offset = (unsigned long)(pFbdev->fb_fix.smem_start) & (~PAGE_MASK);
    pFbdev->fb_mem = (unsigned long int)mmap(NULL, pFbdev->fb_fix.smem_len +
            pFbdev->fb_mem_offset,
            PROT_READ | PROT_WRITE, MAP_SHARED, pFbdev->fb, 0);
    if (-1L == (long) pFbdev->fb_mem)
    {
        printf("mmap error! mem:%ld offset:%ld\n", pFbdev->fb_mem,
                pFbdev->fb_mem_offset);
        return -1;
    }
    return 0;
}
void fb_close(PFBDEV pFbdev)
{
    close(pFbdev->fb);
    pFbdev->fb=-1;
}
void fb_memset (void *addr, int c, size_t len)
{
    memset(addr, c, len);
}
void draw_dot (PFBDEV pFbdev, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b)
{
    unsigned int offset;
    if (-1L == (long) pFbdev->fb_mem)
    {
        printf("mmap error! mem:%ld offset:%ld\n", pFbdev->fb_mem,
                pFbdev->fb_mem_offset);
        return;
    }
//    offset = y*pFbdev->fb_fix.line_length+4*x;
    offset = y*pFbdev->fb_fix.line_length+2*x;
    fb_memset((void*)(pFbdev->fb_mem + pFbdev->fb_mem_offset + offset), b, 1);
    fb_memset((void*)(pFbdev->fb_mem + pFbdev->fb_mem_offset + offset + 1), g, 1);
//    fb_memset((void*)(pFbdev->fb_mem + pFbdev->fb_mem_offset + offset + 2), r, 1);
//    fb_memset((void*)(pFbdev->fb_mem + pFbdev->fb_mem_offset + offset + 3), 0x00, 1);
}
int main(void)
{
    FBDEV fb;
    memset(&fb, 0, sizeof(FBDEV));
    strcpy(fb.dev, "/dev/graphics/fb0");
    if(fb_open(&fb)==-1)
    {
        printf("open frame buffer error\n");
        return 0;
    }
    printf("depth(bits per pixel) =  %d\n",fb.fb_var.bits_per_pixel);
    printf("smemlen = %d\n",fb.fb_fix.smem_len);
    printf("fix_line(in byte) = %d\n",fb.fb_fix.line_length);
    int i,j;
    for(i=100; i < 300; ++i)
        for(j = 80;j < 100;++j)
        {
            draw_dot(&fb,i,j,0xff,0xff,0xff);
        }
    fb_close(&fb);
    return 0;
}
