#include	<stdio.h>
#include	<dlfcn.h>

#include	<Processing.NDI.Lib.h>
#include	<Processing.NDI.DynamicLoad.h>

#include	<opencv2/imgcodecs.hpp>
#include	<opencv2/imgproc.hpp>
#include	<opencv2/videoio.hpp>
#include	<opencv2/highgui.hpp>
#include	<opencv2/video.hpp>

#define	NDI_SEND_VIDEO_FORMAT_BGRX	0
#define	NDI_SEND_VIDEO_FORMAT_RGBX	1
#define	NDI_SEND_VIDEO_FORMAT_I420	2
#define	NDI_SEND_VIDEO_FORMAT_UYVA	3
#define	NDI_SEND_VIDEO_FORMAT_UYVY	4

using namespace cv;

const NDIlib_v3							*NDILib;
void									*hNDILib;
NDIlib_send_instance_t					ndi_send;
NDIlib_video_frame_v2_t					ndi_video_frame;

void rgb_to_yuv422_uyvy(const cv::Mat& rgb, cv::Mat& yuv) 
{
	assert(rgb.size() == yuv.size() &&
		   rgb.depth() == CV_8U &&
		   rgb.channels() == 3 &&
		   yuv.depth() == CV_8U &&
		   yuv.channels() == 2);
	for(int ih = 0; ih < rgb.rows; ih++) 
	{
		const uint8_t* rgbRowPtr = rgb.ptr<uint8_t>(ih);
		uint8_t* yuvRowPtr = yuv.ptr<uint8_t>(ih);

		for(int iw = 0; iw < rgb.cols; iw = iw + 2) 
		{
			const int rgbColIdxBytes = iw * rgb.elemSize();
			const int yuvColIdxBytes = iw * yuv.elemSize();

			const uint8_t R1 = rgbRowPtr[rgbColIdxBytes + 0];
			const uint8_t G1 = rgbRowPtr[rgbColIdxBytes + 1];
			const uint8_t B1 = rgbRowPtr[rgbColIdxBytes + 2];
			const uint8_t R2 = rgbRowPtr[rgbColIdxBytes + 3];
			const uint8_t G2 = rgbRowPtr[rgbColIdxBytes + 4];
			const uint8_t B2 = rgbRowPtr[rgbColIdxBytes + 5];

			const int Y  =  (0.257f * R1) + (0.504f * G1) + (0.098f * B1) + 16.0f ;
			const int U  = -(0.148f * R1) - (0.291f * G1) + (0.439f * B1) + 128.0f;
			const int V  =  (0.439f * R1) - (0.368f * G1) - (0.071f * B1) + 128.0f;
			const int Y2 =  (0.257f * R2) + (0.504f * G2) + (0.098f * B2) + 16.0f ;

			yuvRowPtr[yuvColIdxBytes + 0] = cv::saturate_cast<uint8_t>(U );
			yuvRowPtr[yuvColIdxBytes + 1] = cv::saturate_cast<uint8_t>(Y );
			yuvRowPtr[yuvColIdxBytes + 2] = cv::saturate_cast<uint8_t>(V );
			yuvRowPtr[yuvColIdxBytes + 3] = cv::saturate_cast<uint8_t>(Y2);
		}
	}
}

void	*BGRA_UYVA(Mat in_mat, int ww, int hh)
{
	Mat uyvy = Mat(hh, ww, CV_8UC2);
	Mat rgb;
	cvtColor(in_mat, rgb, COLOR_BGRA2BGR);
	rgb_to_yuv422_uyvy(rgb, uyvy); 

	void *result = (void *)malloc(((ww * 2) * hh) + (ww * hh));
	if(result != NULL)
	{
		memcpy(result, uyvy.ptr(), ((ww * 2) * hh));
		char *cp = (char *)result + ((ww * 2) * hh);
		memset(cp, 255, (ww * hh));
	}
	return(result);
}

void	BGRA_UYVY(Mat in_mat, Mat& out_mat, int ww, int hh)
{
	Mat uyvy = Mat(hh, ww, CV_8UC2);
	Mat rgb;
	cvtColor(in_mat, rgb, COLOR_BGRA2BGR);
	rgb_to_yuv422_uyvy(rgb, uyvy); 
	uyvy.copyTo(out_mat);
}

void	stream_to_ndi(Mat mat, char *stream_name, int format)
{
int	loop;

	if(NDILib != NULL)
	{
		int width = mat.cols;
		int height = mat.rows;
		if(ndi_send == NULL)
		{
			NDIlib_send_create_t NDI_send_create_desc;
			if(strlen(stream_name) > 0)
			{
				NDI_send_create_desc.p_ndi_name = stream_name;
			}
			else
			{
				NDI_send_create_desc.p_ndi_name = "Cow NDI";
			}
			ndi_send = NDILib->NDIlib_send_create(&NDI_send_create_desc);
			ndi_video_frame.xres = width;
			ndi_video_frame.yres = height;
			if(format == NDI_SEND_VIDEO_FORMAT_BGRX)
			{
				ndi_video_frame.FourCC = NDIlib_FourCC_type_BGRX;
			}
			else if(format == NDI_SEND_VIDEO_FORMAT_RGBX)
			{
				ndi_video_frame.FourCC = NDIlib_FourCC_type_RGBX;
			}
			else if(format == NDI_SEND_VIDEO_FORMAT_I420)
			{
				ndi_video_frame.FourCC = NDIlib_FourCC_type_I420;
			}
			else if(format == NDI_SEND_VIDEO_FORMAT_UYVA)
			{
				ndi_video_frame.FourCC = NDIlib_FourCC_type_UYVA;
			}
			else if(format == NDI_SEND_VIDEO_FORMAT_UYVY)
			{
				ndi_video_frame.FourCC = NDIlib_FourCC_type_UYVY;
			}
		}
		if(ndi_send != NULL)
		{
			void *result = NULL;
			Mat local_mat;
			if(format == NDI_SEND_VIDEO_FORMAT_BGRX)
			{
				Mat local_mat;
				cvtColor(mat, local_mat, COLOR_BGRA2RGBA);
				ndi_video_frame.p_data = local_mat.ptr();
			}
			else if(format == NDI_SEND_VIDEO_FORMAT_RGBX)
			{
				ndi_video_frame.p_data = mat.ptr();
			}
			else if(format == NDI_SEND_VIDEO_FORMAT_I420)
			{
				cvtColor(mat, local_mat, COLOR_RGB2YUV_I420);
				ndi_video_frame.p_data = local_mat.ptr();
			}
			else if(format == NDI_SEND_VIDEO_FORMAT_UYVA)
			{
				result = BGRA_UYVA(mat, width, height);
				if(result != NULL)
				{
					ndi_video_frame.p_data = (unsigned char *)result;
				}
				else
				{
					fprintf(stderr, "Error: Frame conversion to UYVA failed\n");
				}
			}
			else if(format == NDI_SEND_VIDEO_FORMAT_UYVY)
			{
				Mat out_mat;
				BGRA_UYVY(mat, out_mat, width, height);
				ndi_video_frame.p_data = (unsigned char *)out_mat.ptr();
			}
			NDILib->NDIlib_send_send_video_v2(ndi_send, &ndi_video_frame);
			if(result != NULL)
			{
				free(result);
			}
		}
		else
		{
			fprintf(stderr, "Error: NDI Send handle is NULL\n");
			exit(-1);
		}
	}
	else
	{
		fprintf(stderr, "Error: NDI Library pointer is NULL\n");
		exit(-1);
	}
}

void	DynamicallyLoadNDILibrary(char *library_path)
{
	hNDILib = dlopen(library_path, RTLD_LOCAL | RTLD_LAZY);
	const NDIlib_v3 *(*NDIlib_v3_load)(void) = NULL;
	if(hNDILib)
	{
		*((void**)&NDIlib_v3_load) = dlsym(hNDILib, "NDIlib_v3_load");
		if(!NDIlib_v3_load)
		{
			if(hNDILib)
			{
				dlclose(hNDILib);
				hNDILib = NULL;
			}
		}
		else
		{
			NDILib = (const NDIlib_v3 *)NDIlib_v3_load();
		}
	}
}

int	main(int argc, char **argv)
{
	int help = 0;
	if(argc > 1)
	{
		if(strcmp(argv[1], "--help") == 0)
		{
			help = 1;
		}
		else
		{
			char *camera_path = argv[1];
			char *stream_name = "Cow NDI";
			int stream_type = NDI_SEND_VIDEO_FORMAT_I420;
			if(argc > 2)
			{
				stream_name = argv[2];
				if(argc > 3)
				{
					stream_type = atoi(argv[3]);
				}
			}
			NDILib = NULL;
			hNDILib = NULL;
			DynamicallyLoadNDILibrary("libndi.so");
			int no_go = 0;
			if(NDILib != NULL)
			{
				if(NDILib->NDIlib_initialize())
				{
					VideoCapture capture(camera_path);
					if(capture.isOpened())
					{
						Mat frame;
						int done = 0;
						while(done == 0)
						{
							capture >> frame;
							cvtColor(frame, frame, COLOR_RGB2BGRA);
						
							stream_to_ndi(frame, "Cow Stream", stream_type);
						}
					}
					else
					{
						fprintf(stderr, "Error: Camera failed to open\n");
					}
				}
				if(NDILib != NULL)
				{
					NDILib->NDIlib_destroy();
				}
			}
			else
			{
				fprintf(stderr, "Error: NDI Library did not load or initialize\n");
			}
			if(hNDILib != NULL)
			{
				dlclose(hNDILib);
			}
		}
	}
	else
	{
		help = 1;
	}
	if(help == 1)
	{
		fprintf(stderr, "Copyright © 2025 - Mark Lasersohn\n");
		fprintf(stderr, "NDI™ is a registered trademark of Vizrt NDI AB\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Syntax: cow_ndi camera_path [ndi_stream_name] [video_format]\n");
		fprintf(stderr, "\tcamera_path is the path to the camera. This may be a device, such as as USB device,\n");
		fprintf(stderr, "\tin the /dev directory, or it may be a video file. It can be any file OpenCV can use\n");
		fprintf(stderr, "\tas a capture device.\n");
		fprintf(stderr, "\tThe optional argument ndi_stream_name is the name listed by NDI™ for the video stream.\n");
		fprintf(stderr, "\tThe optional argument video_format, is an integer ranging from 0 to 4, used to specify\n");
		fprintf(stderr, "\tthe NDI™ video format for the stream:\n");
		fprintf(stderr, "\t\t0 = BGRX\n");
		fprintf(stderr, "\t\t1 = RGBX\n");
		fprintf(stderr, "\t\t2 = I420\n");
		fprintf(stderr, "\t\t3 = UYVA\n");
		fprintf(stderr, "\t\t4 = UYVY\n");
		fprintf(stderr, "\tI420 is the default.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Examples:\n");
		fprintf(stderr, "\tcow_ndi /dev/video0 ; Stream name defaults to 'Cow NDI' and frame format to I420\n");
		fprintf(stderr, "\tcow_ndi /dev/video0 MyStreamName ; Frame format defaults to I420\n");
		fprintf(stderr, "\tcow_ndi /dev/video0 MyStreamName 3 ; Stream will use UYVA video frame format\n");
	}
}
