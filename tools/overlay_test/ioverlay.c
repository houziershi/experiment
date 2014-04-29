#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/fb.h>

#define V4L_CAP_NAME "/dev/video0"
#define V4L_OVL_NAME "/dev/video16"

#define MXCFB_SET_GBL_ALPHA     _IOW('F', 0x21, struct mxcfb_gbl_alpha)
#define MXCFB_SET_CLR_KEY       _IOW('F', 0x22, struct mxcfb_color_key)

struct mxcfb_gbl_alpha {
	int enable;
	int alpha;
};

int main()
{
	//Open v4l2 device
	int fd = open(V4L_CAP_NAME, O_RDWR);
	int on = 1;
	if(fd < 0) {
		printf("Error!Open v4l device %s failed",V4L_CAP_NAME);
		return -1;
	}

#if 1
	int fd0 = open("/dev/graphics/fb0", O_RDWR);

	int layer = 3;
	if (ioctl(fd, VIDIOC_S_OUTPUT, &layer) < 0) {
		close(fd);
		close(fd0);
		return -1;
	}

	int input = 1;
	if (ioctl(fd, VIDIOC_S_INPUT, &input) < 0) {
		printf("set input failed");
		return -1;
	}

	v4l2_std_id id;
	if (ioctl(fd, VIDIOC_G_STD, &id) < 0)
	{
		printf("VIDIOC_G_STD failed\n");
		return -1;
	}
	printf("VIDIOC_S_STD \n");
	if (ioctl(fd, VIDIOC_S_STD, &id) < 0)
	{
		printf("VIDIOC_S_STD failed\n");
		return -1;
	}

	struct v4l2_format fmt;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	fmt.fmt.pix.width = 720&0xFFFFFFF8;
	fmt.fmt.pix.height = 480&0xFFFFFFF8;
	fmt.fmt.pix.bytesperline = fmt.fmt.pix.width;
	fmt.fmt.pix.priv = 0;
	fmt.fmt.pix.sizeimage = 0;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED_TB;
	if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
		printf("set format failed\n");
		printf("new...\n");
		return -5;
	}

	struct v4l2_crop crop;
	/* set the image rectangle of the display by 
	   setting the appropriate parameters */
	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	crop.c.width = 720;
	crop.c.height = 240;
	crop.c.top = 0;
	crop.c.left = 0;
	if ( ioctl(fd, VIDIOC_S_CROP, &crop) < 0) {
		printf("Error!VIDIOC_S_CROP getting failed for dev %s",V4L_OVL_NAME);
		return -1;
	}

	if (ioctl(fd, VIDIOC_OVERLAY, &on) < 0) {
		printf("Error!VIDIOC_OVERLAY getting failed for dev %s",V4L_CAP_NAME);
		return -1;
	}

	struct v4l2_streamparm parm;
	//hard code here to do a walk around.
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = 30;
	parm.parm.capture.capturemode = 0;
	if (ioctl(fd, VIDIOC_S_PARM, &parm) < 0) {
	printf("%s:%d  VIDIOC_S_PARM failed\n", __FUNCTION__,__LINE__);
	return -6;
	}

	struct mxcfb_gbl_alpha gbl_alpha;
	gbl_alpha.alpha = 0;
	gbl_alpha.enable = 1;
	if (ioctl(fd0, MXCFB_SET_GBL_ALPHA,
				&gbl_alpha) < 0) {
		return -1;
	}

#endif
	while(1)
	{
		usleep(50);
	}
}
