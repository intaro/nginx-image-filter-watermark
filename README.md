Nginx-image-filter-watermark
============================

Patched `image_filter_module` for Nginx with watermark ability. Patch based on [http_image_filter_module](http://nginx.org/en/docs/http/ngx_http_image_filter_module.html)

Should work properly on nginx >= 1.11.6.

### Available options:

```
image_filter watermark;

image_filter_watermark_width_from 300;
image_filter_watermark_height_from 400;
    
image_filter_watermark "PATH_TO_FILE";
image_filter_watermark_position center-center; # top-left|top-right|bottom-right|bottom-left|right-center|left-center|bottom-center|top-center|center-center|center-random`


image_filter_watermark_text "Text"; #Text label
image_filter_watermark_font "/tmp/font.TTF:italic:bold"; #Optional fontfile or font "times:bold:italic"
image_filter_watermark_size 8.5; #Font Size
image_filter_watermark_colorR "0"; #Font Color R
image_filter_watermark_colorB "0"; #Font Color B
image_filter_watermark_colorG "0"; #Font Color G
image_filter_watermark_fontposition top-left; #Font location top-left|top-right|bottom-right|bottom-left|right-center|left-center|bottom-center|top-center|center-center|center-random`


```

`image_filter_watermark_width_from` - Minimal width image (after resize and crop) of when to use watermark.
`image_filter_watermark_height_from` - Minimal height image (after resize and crop) of when to use watermark.

If width or height image (after resize and crop) more then `image_filter_watermark_height_from` or `image_filter_watermark_width_from` then image gets watermark.

`image_filter_watermark` - path to watermark file.
`image_filter_watermark_position` - position of watermark, available values are `top-left|top-right|bottom-right|bottom-left|right-center|left-center|bottom-center|top-center|center-center|center-random`.

### Install

Get source code and replace  module file in
`nginx/src/http/modules/http_image_filter_module.c`

Build nginx with module.

```
/configure --with-http_image_filter_module
make
make install
```


### Example Usage

Base Usage:

```
    location /img/ {
        image_filter watermark;

        image_filter_watermark "PATH_TO_FILE";
        image_filter_watermark_position center-center;
    }
```

Usage with resize and crop:

```
   location ~ ^/r/(\d+|-)x(\d+|-)/c/(\d+|-)x(\d+|-)/(.+) {
       set                         $resize_width  $1;
       set                         $resize_height $2;
       set                         $crop_width  $3;
       set                         $crop_height $4;

       alias                       /PATH_TO_STATIC/web/$5;
       try_files                   "" @404;

       image_filter                resize $resize_width $resize_height;
       image_filter                crop   $crop_width $crop_height;

       image_filter_jpeg_quality   95;
       image_filter_buffer         2M;

       image_filter_watermark_width_from 400;   # Minimal width (after resize) of when to use watermark
       image_filter_watermark_height_from 400;  # Minimal height (after resize) of when to use watermark

       image_filter_watermark "PATH_TO_FILE";
       image_filter_watermark_position center-center;
   }
```

### Example test memory leaks

```
ab -n 3000 -c 10 -k  http://image-resize.local/r/500x-/some-file.jpg
```

While there is a load test,  track at how the RAM behaves
