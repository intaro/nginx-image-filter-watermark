Nginx-image-filter-watermark
============================

Patched image_filter module for Nginx with watermark ability

Patch based on [http_image_filter_module](http://nginx.org/en/docs/http/ngx_http_image_filter_module.html)

Tested on nginx-1.5.9



### Available options:

```
    image_filter                watermark;

    image_filter_watermark_width_from 300;
    image_filter_watermark_height_from 400;
    
    image_filter_watermark "PATH_TO_FILE";
    image_filter_watermark_position bottom-right; # top-left|top-right|bottom-right|bottom-left
```

image_filter_watermark_width_from -  Minimal width image (after resize and crop) of when to use watermark.

image_filter_watermark_height_from -  Minimal height image (after resize and crop) of when to use watermark.

if  width or height image (after resize and crop) more then image_filter_watermark_height_from or image_filter_watermark_width_from then image get watermark

image_filter_watermark -  path to watermark file

image_filter_watermark_position - position watermark, available: top-left|top-right|bottom-right|bottom-left

### Install

Get source code and replace  module file in
nginx/src/http/modules/http_image_filter_module.c

Build nginx with module

```
/configure  --with-http_image_filter_module
make
make install
```


### Example Usage

Base Usage:

```
    location /img/) {
        image_filter                watermark;

        image_filter_watermark "PATH_TO_FILE";
        image_filter_watermark_position bottom-right; # top-left|top-right|bottom-right|bottom-left
    }
```

Usage with resize and crop:

```
   location ~ ^/r/(\d+|-)x(\d+|-)/c/(\d+|-)x(\d+|-)/(.+) {
       set                         $resize_width  $1;
       set                         $resize_height $2;
       set                         $crop_width  $3;
       set                         $crop_height $4;

       alias                       /Users/goshan/Sites/Zot/Zot/web/$5;
       try_files                   "" @404;

       image_filter                resize $resize_width $resize_height;
       image_filter                crop   $crop_width $crop_height;

       image_filter_jpeg_quality   95;
       image_filter_buffer         2M;

       image_filter_watermark_width_from 400;   # Minimal width (after resize) of when to use watermark
       image_filter_watermark_height_from 400;  # Minimal height (after resize) of when to use watermark

       image_filter_watermark "PATH_TO_FILE";
       image_filter_watermark_position bottom-right; # top-left|top-right|bottom-right|bottom-left
   }
```