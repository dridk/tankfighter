#ifndef __IMAGE_HELPER_H__
#define __IMAGE_HELPER_H__

#ifdef __cplusplus
extern "C" {
#endif
int up_scale_image
	(
		const unsigned char* const orig,
		int width, int height, int channels,
		unsigned char* resampled,
		int resampled_width, int resampled_height
	);
int mipmap_image
	(
		const unsigned char* const orig,
		int width, int height, int channels,
		unsigned char* resampled,
		int block_size_x, int block_size_y
	);
int scale_image
	(
		const unsigned char* const orig,
		int width, int height, int channels,
		unsigned char* resampled,
		int rwidth, int rheight
	);
#ifdef __cplusplus
}
#endif

#endif
