#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <pthread.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <math.h>

#define WIDTH		720 /*320*/
#define HEIGHT		480 /*240*/

int frames = 150;
int crop_top;

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define MXCFB_SET_GBL_ALPHA     _IOW('F', 0x21, struct mxcfb_gbl_alpha)

struct buffer {
	void *start;
	size_t length;
};

struct buf_info {
	unsigned int length;
	char *start;
};

struct mxcfb_gbl_alpha {
	int enable;
	int alpha;
};

static char *dev_name = NULL;
static int fd = -1;
struct buffer *buffers = NULL;
static unsigned int n_buffers = 0;
static int time_in_sec_capture = 5;
static int fbfd = -1, fdo = -1;
static char *fbp = NULL;
struct v4l2_buffer v4l_buffers[3];
char *v4lbuf_addr[3];
static int overlay_fd = 0;
struct buf_info *buff_info;
struct screen {
	char *buf;
	int bytes_per_line;
	int height;
};
struct screen screen;

struct screen *sc = &screen;

static void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s/n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}
static int xioctl(int fd, int request, void *arg)
{
	int r;
	do
		r = ioctl(fd, request, arg);
	while (-1 == r && EINTR == errno);
	return r;
}

inline int clip(int value, int min, int max)
{
	return (value > max ? max : value < min ? min : value);
}
#define OVERLAY 1

#define ALPHA 1
static int read_frame(void)
{
	struct v4l2_buffer buf;
	unsigned int i;
	static int arc = 1;

	static int ll = 0;
	CLEAR(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

#if 1
	if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
		switch (errno) {
		case EAGAIN:
			return 0;
		case EIO:

		default:
			errno_exit("VIDIOC_DQBUF");
		}
	}
#endif
	assert(buf.index < n_buffers); 
	#if !OVERLAY
	process_image(buffers[buf.index].start);
	#else

	struct v4l2_buffer overlay_buf;
	int ret = -1, j, m;
#if 1
	overlay_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	overlay_buf.memory = V4L2_MEMORY_MMAP;
#endif
	ret = ioctl(overlay_fd, VIDIOC_DQBUF, &overlay_buf);
	if(ret < 0){
		perror("VIDIOC_DQBUF\n");
		for (j = 0; j < 3; j++)
			munmap(buff_info[j].start, buff_info[j].length);
		close(overlay_fd);
		exit(0);
	}
#if 1
	memcpy(buff_info[overlay_buf.index].start, buffers[buf.index].start, buffers[buf.index].length);
#endif
	
#if 1
	ret = ioctl(overlay_fd, VIDIOC_QBUF, &overlay_buf);
	if(ret < 0){
		perror("VIDIOC_QBUF\n");
		for (j = 0; j < 3; j++)
			munmap(buff_info[j].start, buff_info[j].length);
		close(overlay_fd);
		exit(0);
	}
#endif

#if ALPHA
	struct mxcfb_gbl_alpha gbl_alpha;
    gbl_alpha.alpha = 0;
    gbl_alpha.enable = 1;
    ioctl(fbfd, MXCFB_SET_GBL_ALPHA, &gbl_alpha);
#endif
	#endif
#if 1	
	if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		errno_exit("VIDIOC_QBUF");
#endif

	return 1;
}

unsigned long  status = 0;
static void run(void)
{
	unsigned int count;
	pthread_t   rd_id;
	pthread_attr_t rd_attr;
	int ret = 0;

	while(1) {
		for(;;)
		{
			fd_set fds;
			struct timeval tv;
			int r;

			if (--frames == 0)
				frames = 150;
			
			FD_ZERO(&fds);
			FD_SET(fd, &fds);

			tv.tv_sec = 2;
			tv.tv_usec = 0;

			r = select(fd + 1, &fds, NULL, NULL, &tv);

			if (-1 == r) {
				if (EINTR == errno)
					continue;
				errno_exit("select");
			}

			if (0 == r) {
				fprintf(stderr, "select timeout/n");
				exit(EXIT_FAILURE);
			}

			if (read_frame())
				break;
		}
		usleep(50);
	}
}

static void stop_capturing(void)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
		errno_exit("VIDIOC_STREAMOFF");
}

static void start_capturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

	for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
		errno_exit("VIDIOC_STREAMON");

}

static void uninit_device(void)
{
	unsigned int i;

	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap(buffers[i].start, buffers[i].length))
			errno_exit("munmap");

	for (i = 0; i < n_buffers; ++i)
		munmap(buff_info[i].start,
				buff_info[i].length);

	free(buffers);
	free(buff_info);
}

static void init_mmap(void)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support memory mapping/n",
				dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 4) {	//if (req.count < 2)  
		fprintf(stderr, "Insufficient buffer memory on %s/n", dev_name);
		exit(EXIT_FAILURE);
	}

	buffers = calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory/n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
		    mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
			 fd, buf.m.offset);
		printf("index:%d, length:%d, offset:%d\n",n_buffers, buf.length, buf.m.offset);
		if (MAP_FAILED == buffers[n_buffers].start)
			printf("map failed here.........\n");
	}

}
#define V4L_DEV_NAME "/dev/video16"

static void init_device(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;
	int ret = 0;

	int layer = 3;
	ret = ioctl(fd, VIDIOC_S_OUTPUT, &layer);
	if (ret < 0) {
		close(fd);
		exit(0);
	}

	int input = 1;
	if (ioctl(fd, VIDIOC_S_INPUT, &input) < 0) {
			fprintf(stderr, "%s is no V4L2 device/n", dev_name);
	}

	v4l2_std_id id;
	if (ioctl(fd, VIDIOC_G_STD, &id) < 0)
	{
			fprintf(stderr, "%s is no V4L2 device/n", dev_name);
	}

	if (ioctl(fd, VIDIOC_S_STD, &id) < 0)
	{
			fprintf(stderr, "%s is no V4L2 device/n", dev_name);
	}

	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device/n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}
	printf("VIDIOC_QUERYCAP:0x%x\n", cap.capabilities);
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device/n", dev_name);
		exit(EXIT_FAILURE);
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf(stderr, "%s does not support streaming i/o/n",
			dev_name);
		exit(EXIT_FAILURE);
	}

	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#if 1
		crop.c = cropcap.defrect;
#endif

		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c.width = 720;
		crop.c.height = 480;
		crop.c.top = crop_top;
		crop.c.left = 0;
		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				break;
			default:
				break;
			}
		}
	} else {
	}
	
	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 720;
	fmt.fmt.pix.height = 480;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED_TB;

	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
		errno_exit("Capture VIDIOC_S_FMT failed.");

	struct v4l2_streamparm parm;
	//hard code here to do a walk around.
	CLEAR(parm);
	int numerator = 1;
	int denominator = 30;
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = numerator;
	parm.parm.capture.timeperframe.denominator = denominator;
	parm.parm.capture.capturemode = 0;
	if (ioctl(fd, VIDIOC_S_PARM, &parm) < 0) {
		parm.parm.capture.timeperframe.numerator = 1;
		parm.parm.capture.timeperframe.denominator = 15;
		if (ioctl(fd, VIDIOC_S_PARM, &parm) < 0){
			printf("%s:%d  VIDIOC_S_PARM failed\n", __FUNCTION__,__LINE__);
			printf("frame timeval is numerator %d, denominator %d",parm.parm.capture.timeperframe.numerator, 
					parm.parm.capture.timeperframe.denominator);
		}
	}

	init_mmap();

}

static void close_device(void)
{
	printf("camera v4l2 close.\n");
#if ALPHA
	struct mxcfb_gbl_alpha gbl_alpha;
    gbl_alpha.alpha = 255;
    gbl_alpha.enable = 1;
    ioctl(fbfd, MXCFB_SET_GBL_ALPHA, &gbl_alpha);
#endif

	if (-1 == close(fd))
		errno_exit("close");
	fd = -1;
	close(fbfd);
}

static void open_device(void)
{
	struct stat st;

	if (-1 == stat(dev_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s/n", dev_name,
			errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device/n", dev_name);
		exit(EXIT_FAILURE);
	}
	//open framebuffer  
	fbfd = open("/dev/graphics/fb0", O_RDWR);
	if (fbfd == -1) {
		printf("Error: cannot open framebuffer device./n");
		exit(EXIT_FAILURE);
	}
	//open camera  
	fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);

	if (-1 == fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s/n", dev_name, errno,
			strerror(errno));
		exit(EXIT_FAILURE);
	}
}

#define LOOPCOUNT	200
#define NUMBUFFERS	3

void init_overlay()
{
	int mode = O_RDWR, c, ret, j, i;
	int index,m = 1,del_cnt=10;
	int fd, ch_no = 0, numbuffers = NUMBUFFERS, a;
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	struct v4l2_format fmt;
	struct timeval before, after, result;
	
	if (numbuffers < 0) {
		printf("Invalid numbufers\n");
		exit(0);
	}
	fd = open(V4L_DEV_NAME, mode);
	if (fd <= 0) {
		printf("Cannot open = %s device\n", V4L_DEV_NAME);
		perror("Error:");
		exit(0);
	}
	overlay_fd = fd;
	
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width = WIDTH;
	fmt.fmt.pix.height = HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED_TB;
        fmt.fmt.pix.priv = 0;
        ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
        if (ret < 0) {
                perror("Output VIDIOC_S_FMT failed.\n");
                close(fd);
                exit(0);
        }


	ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		perror("VIDIOC_G_FMT\n");
		close(fd);
		exit(0);
	}

	printf("fmt.fmt.pix.bytesperline:%d\n", fmt.fmt.pix.bytesperline);
	printf("getfmt\n");
	
	req.count = numbuffers;
	req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	req.memory = V4L2_MEMORY_MMAP;
	
	ret = ioctl(fd, VIDIOC_REQBUFS, &req);
	if (ret < 0) {
		perror("cannot allocate memory\n");
		close(fd);
		exit(0);
	}
	printf("reqbufs = %d\n",req.count);

	buff_info =
		(struct buf_info *) malloc(sizeof(struct buf_info) *
				req.count);
	if (!buff_info) {
		printf("cannot allocate memory for buff_info\n");
		close(fd);
		exit(0);
	}

	memset(&buf, 0, sizeof(buf));
	for (i = 0; i < req.count; i++) {
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.index = i;
		buf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
		if (ret < 0) {
			perror("VIDIOC_QUERYCAP\n");
			for (j = 0; j < i; j++)
				munmap(buff_info[j].start,
						buff_info[j].length);
			close(fd);
			exit(0);
		}
		buff_info[i].length = buf.length;

		buff_info[i].start =
			mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
					MAP_SHARED, fd, buf.m.offset);

		if (buff_info[i].start == MAP_FAILED) {
			printf("Cannot mmap = %d buffer\n", i);
			for (j = 0; j < i; j++)
				munmap(buff_info[j].start,
						buff_info[j].length);
			close(fd);
			exit(0);
		}
		memset(buff_info[i].start, 0x80, buff_info[i].length);

		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			perror("VIDIOC_QBUF\n");
			for (j = 0; j < req.count; j++)
				munmap(buff_info[j].start,
						buff_info[j].length);
			exit(0);
		}
	}

	memset(&buf, 0, sizeof(buf));

	a = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(fd, VIDIOC_STREAMON, &a);
	if (ret < 0) {
		perror("VIDIOC_STREAMON\n");
		for (i = 0; i < req.count; i++ ){
			printf("Freeing buffer %d\n",i);
			munmap(buff_info[i].start, buff_info[i].length);
		}
		close(fd);
		exit(0);
	}
}

static void usage(FILE * fp, int argc, char **argv)
{
	fprintf(fp,
		"Usage: %s [options]/n/n"
		"Options:/n"
		"-d | --device name Video device name [/dev/video]/n"
		"-h | --help Print this message/n"
		"-t | --how long will display in seconds/n" "", argv[0]);
}

static const char short_options[] = "d:ht:";
static const struct option long_options[] = {
	{"device", required_argument, NULL, 'd'},
	{"help", no_argument, NULL, 'h'},
	{"time", no_argument, NULL, 't'},
	{0, 0, 0, 0}
};

void v4l2_camera_on(void)
{
	open_device();
	init_device();
#if OVERLAY
	init_overlay();
#endif
	start_capturing();
	
	run();

}
void v4l2_camera_off(void)
{

	printf("camera v4l2 off.\n");
	uninit_device();

	close_device();

	stop_capturing();
	exit(EXIT_SUCCESS);
}

typedef struct  _sig_fd_sets {
	int tw8816_fd;
}sig_fd_sets;
sig_fd_sets d_sig_fd_sets;
static pthread_mutex_t g_rd_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv)
{
	int ret = 0;
	fd_set rfds;
	pthread_t   rd_id;
	pthread_attr_t rd_attr;
	dev_name = "/dev/video0";

	if(argc == 2)
	{
		crop_top = atoi(argv[1]);
	}

	v4l2_camera_on();
	v4l2_camera_off();

	close(d_sig_fd_sets.tw8816_fd);
	return 0;
}

