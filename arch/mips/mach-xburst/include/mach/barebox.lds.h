#define PRE_IMAGE \
	. = 0x80000000;					\
	.pre_image : {					\
		_start_flash = .;			\
		KEEP(*(.flash_header_start*))		\
		KEEP(*(.flash_header*))			\
	}
