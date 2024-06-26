/*
Platform specific code for displaying a bitmap image in some format that might be
specific to the target platform.

img_base is a pointer to the start of the image data
*/

#ifdef TARGET_ZXSPEC48
void show_image(unsigned char *img_base, unsigned img_len){
    // Decompress the image directly in to the frame buffer
    dzx0_standard(img_base, (unsigned char *)0x4000);
}
#endif

#ifdef TARGET_PC_LINUX
void show_image(unsigned char *img_base, unsigned int img_len){
    fflush(stdout);
    fwrite(img_base, img_len, 1, stdout);
}
#endif

#if defined(TARGET_PC_MSDOS) || defined(TARGET_PC_MSDOS_TEXT)
void show_image(unsigned char *img_base, unsigned int img_len){
    // TODO
}
#endif
