--- src/http/modules/ngx_http_image_filter_module.c.orig
+++ src/http/modules/ngx_http_image_filter_module.c
@@ -18,7 +18,7 @@
 #define NGX_HTTP_IMAGE_RESIZE    3
 #define NGX_HTTP_IMAGE_CROP      4
 #define NGX_HTTP_IMAGE_ROTATE    5
-
+#define NGX_HTTP_IMAGE_WATERMARK 6
 
 #define NGX_HTTP_IMAGE_START     0
 #define NGX_HTTP_IMAGE_READ      1
@@ -37,21 +37,28 @@
 
 
 typedef struct {
-    ngx_uint_t                   filter;
-    ngx_uint_t                   width;
-    ngx_uint_t                   height;
-    ngx_uint_t                   angle;
-    ngx_uint_t                   jpeg_quality;
-    ngx_uint_t                   sharpen;
+    ngx_uint_t          filter;
+    ngx_uint_t          width;
+    ngx_uint_t          height;
+    ngx_uint_t          angle;
+    ngx_uint_t          jpeg_quality;
+    ngx_uint_t          sharpen;
 
-    ngx_flag_t                   transparency;
-    ngx_flag_t                   interlace;
+    ngx_flag_t          transparency;
+    ngx_flag_t          interlace;
+
+    ngx_str_t           watermark;  // watermark file url
+    ngx_str_t           watermark_position; // top-left|top-right|bottom-right|bottom-left
+    ngx_int_t           watermark_width_from; // width from use watermark
+    ngx_int_t           watermark_height_from; // height from use watermark
 
     ngx_http_complex_value_t    *wcv;
     ngx_http_complex_value_t    *hcv;
     ngx_http_complex_value_t    *acv;
     ngx_http_complex_value_t    *jqcv;
     ngx_http_complex_value_t    *shcv;
+    ngx_http_complex_value_t    *wmcv;
+    ngx_http_complex_value_t    *wmpcv;
 
     size_t                       buffer_size;
 } ngx_http_image_filter_conf_t;
@@ -144,7 +151,7 @@ static ngx_command_t  ngx_http_image_filter_commands[] = {
       offsetof(ngx_http_image_filter_conf_t, transparency),
       NULL },
 
-    { ngx_string("image_filter_interlace"),
+   { ngx_string("image_filter_interlace"),
       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
       ngx_conf_set_flag_slot,
       NGX_HTTP_LOC_CONF_OFFSET,
@@ -157,6 +164,30 @@ static ngx_command_t  ngx_http_image_filter_commands[] = {
       NGX_HTTP_LOC_CONF_OFFSET,
       offsetof(ngx_http_image_filter_conf_t, buffer_size),
       NULL },
+    { ngx_string("image_filter_watermark"),
+      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
+      ngx_http_set_complex_value_slot,
+      NGX_HTTP_LOC_CONF_OFFSET,
+      offsetof(ngx_http_image_filter_conf_t, wmcv),
+      NULL },
+    { ngx_string("image_filter_watermark_position"),
+      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
+      ngx_http_set_complex_value_slot,
+      NGX_HTTP_LOC_CONF_OFFSET,
+      offsetof(ngx_http_image_filter_conf_t, wmpcv),
+      NULL },
+    { ngx_string("image_filter_watermark_height_from"),
+      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
+      ngx_conf_set_num_slot,
+      NGX_HTTP_LOC_CONF_OFFSET,
+      offsetof(ngx_http_image_filter_conf_t, watermark_height_from),
+      NULL },
+    { ngx_string("image_filter_watermark_width_from"),
+      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
+      ngx_conf_set_num_slot,
+      NGX_HTTP_LOC_CONF_OFFSET,
+      offsetof(ngx_http_image_filter_conf_t, watermark_width_from),
+      NULL },
 
       ngx_null_command
 };
@@ -500,6 +531,23 @@ ngx_http_image_read(ngx_http_request_t *r, ngx_chain_t *in)
     return NGX_AGAIN;
 }
 
+static ngx_str_t
+ngx_http_image_filter_get_str_value(ngx_http_request_t *r,
+    ngx_http_complex_value_t *cv, ngx_str_t v)
+{
+    ngx_str_t  val;
+    
+    if (cv == NULL) {
+        return v;
+    }
+
+    if (ngx_http_complex_value(r, cv, &val) != NGX_OK) {
+        return val;
+    }
+
+    return val;
+}
+
 
 static ngx_buf_t *
 ngx_http_image_process(ngx_http_request_t *r)
@@ -508,45 +556,65 @@ ngx_http_image_process(ngx_http_request_t *r)
     ngx_http_image_filter_ctx_t   *ctx;
     ngx_http_image_filter_conf_t  *conf;
 
-    r->connection->buffered &= ~NGX_HTTP_IMAGE_BUFFERED;
-
-    ctx = ngx_http_get_module_ctx(r, ngx_http_image_filter_module);
+	r->connection->buffered &= ~NGX_HTTP_IMAGE_BUFFERED;
+	ctx = ngx_http_get_module_ctx(r, ngx_http_image_filter_module);
 
     rc = ngx_http_image_size(r, ctx);
 
     conf = ngx_http_get_module_loc_conf(r, ngx_http_image_filter_module);
-
-    if (conf->filter == NGX_HTTP_IMAGE_SIZE) {
+	if (conf->filter == NGX_HTTP_IMAGE_SIZE) {
         return ngx_http_image_json(r, rc == NGX_OK ? ctx : NULL);
     }
-
-    ctx->angle = ngx_http_image_filter_get_value(r, conf->acv, conf->angle);
+	ctx->angle = ngx_http_image_filter_get_value(r, conf->acv, conf->angle);
 
     if (conf->filter == NGX_HTTP_IMAGE_ROTATE) {
 
-        if (ctx->angle != 90 && ctx->angle != 180 && ctx->angle != 270) {
+	    if (ctx->angle != 90 && ctx->angle != 180 && ctx->angle != 270) {
             return NULL;
         }
-
-        return ngx_http_image_resize(r, ctx);
-    }
-
-    ctx->max_width = ngx_http_image_filter_get_value(r, conf->wcv, conf->width);
-    if (ctx->max_width == 0) {
+	    return ngx_http_image_resize(r, ctx);
+	}
+
+	if (conf->wmcv || conf->watermark.data) {
+		ngx_str_t  watermark_value;
+		watermark_value = ngx_http_image_filter_get_str_value(r, conf->wmcv, conf->watermark);
+		conf->watermark.data = ngx_pcalloc(r->pool, watermark_value.len + 1);
+		ngx_cpystrn(conf->watermark.data, watermark_value.data, watermark_value.len+1);
+
+		conf->watermark.len = watermark_value.len;
+
+		ngx_str_t  watermark_position_value;
+		watermark_position_value = ngx_http_image_filter_get_str_value(r, conf->wmpcv, conf->watermark_position);
+		conf->watermark_position.data = ngx_pcalloc(r->pool, watermark_position_value.len + 1);
+		ngx_cpystrn(conf->watermark_position.data, watermark_position_value.data, watermark_position_value.len+1);
+	
+		conf->watermark_position.len = watermark_position_value.len;
+
+		if (conf->filter == NGX_HTTP_IMAGE_WATERMARK) {
+			if (!conf->watermark.data) {
+				return NULL;
+			}
+	
+			return ngx_http_image_resize(r, ctx);
+		}
+	}
+	ctx->max_width = ngx_http_image_filter_get_value(r, conf->wcv, conf->width);
+
+	if (ctx->max_width == 0) {
         return NULL;
     }
 
-    ctx->max_height = ngx_http_image_filter_get_value(r, conf->hcv,
-                                                      conf->height);
-    if (ctx->max_height == 0) {
+    ctx->max_height = ngx_http_image_filter_get_value(r, conf->hcv, conf->height);
+	if (ctx->max_height == 0) {
         return NULL;
     }
 
-    if (rc == NGX_OK
+	if (rc == NGX_OK
         && ctx->width <= ctx->max_width
         && ctx->height <= ctx->max_height
         && ctx->angle == 0
-        && !ctx->force)
+        && !ctx->force
+        && !conf->watermark.data)
     {
         return ngx_http_image_asis(r, ctx);
     }
@@ -737,7 +805,7 @@ ngx_http_image_size(ngx_http_request_t *r, ngx_http_image_filter_ctx_t *ctx)
     }
 
     ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
-                   "image size: %d x %d", (int) width, (int) height);
+                   "image size: %d x %d", width, height);
 
     ctx->width = width;
     ctx->height = height;
@@ -773,7 +841,8 @@ ngx_http_image_resize(ngx_http_request_t *r, ngx_http_image_filter_ctx_t *ctx)
     if (!ctx->force
         && ctx->angle == 0
         && (ngx_uint_t) sx <= ctx->max_width
-        && (ngx_uint_t) sy <= ctx->max_height)
+        && (ngx_uint_t) sy <= ctx->max_height
+        && !conf->watermark.data)
     {
         gdImageDestroy(src);
         return ngx_http_image_asis(r, ctx);
@@ -827,6 +896,8 @@ transparent:
 
         resize = 0;
 
+    } else if (conf->filter == NGX_HTTP_IMAGE_WATERMARK) {
+        resize = 0;
     } else { /* NGX_HTTP_IMAGE_CROP */
 
         resize = 0;
@@ -972,6 +1043,92 @@ transparent:
         gdImageColorTransparent(dst, gdImageColorExact(dst, red, green, blue));
     }
 
+    if (conf->watermark.data) {
+            int min_w, min_h;
+
+            min_w=ctx->max_width;
+            min_h=ctx->max_height;
+
+            if (!ctx->max_width && !ctx->max_height){
+                min_w=sx;
+                min_h=sy;
+            }
+
+            if ( min_w >= conf->watermark_width_from &&
+                  min_h >= conf->watermark_height_from){
+
+                FILE *watermark_file = fopen((const char *)conf->watermark.data, "r");
+
+                if (watermark_file) {
+                    gdImagePtr watermark, watermark_mix;
+                    ngx_int_t wdx = 0, wdy = 0;
+
+                    watermark = gdImageCreateFromPng(watermark_file);
+
+                    if(watermark != NULL) {
+                        watermark_mix = gdImageCreateTrueColor(watermark->sx, watermark->sy);
+                        if (ngx_strcmp(conf->watermark_position.data, "bottom-right") == 0) {
+                            wdx = (int)dst->sx - watermark->sx - 10;
+                            wdy = (int)dst->sy - watermark->sy - 10;
+                        } else if (ngx_strcmp(conf->watermark_position.data, "top-left") == 0) {
+                            wdx = wdy = 10;
+                        } else if (ngx_strcmp(conf->watermark_position.data, "top-right") == 0) {
+                            wdx = (int)dst->sx - watermark->sx - 10;
+                            wdy = 10;
+                        } else if (ngx_strcmp(conf->watermark_position.data, "bottom-left") == 0) {
+                            wdx = 10;
+                            wdy = (int)dst->sy - watermark->sy - 10;
+                        }else if (ngx_strcmp(conf->watermark_position.data, "top-center") == 0) {
+                            wdy = 10;
+                            wdx = (int)dst->sx/2 - (int)watermark->sx/2;
+                        }else if (ngx_strcmp(conf->watermark_position.data, "bottom-center") == 0) {
+                            wdx = (int)dst->sx/2 - (int)watermark->sx/2;
+                            wdy = (int)dst->sy - watermark->sy - 10;
+                        }else if (ngx_strcmp(conf->watermark_position.data, "left-center") == 0) {
+                            wdx = 10;
+                            wdy = (int)dst->sy/2 - (int)watermark->sy/2;
+                        }else if (ngx_strcmp(conf->watermark_position.data, "right-center") == 0) {
+                            wdx = (int)dst->sx - watermark->sx - 10;
+                            wdy = (int)dst->sy/2 - (int)watermark->sy/2;
+                        }else if (ngx_strcmp(conf->watermark_position.data, "center-center") == 0) {
+                            wdx = (int)dst->sx/2 - (int)watermark->sx/2;
+                            wdy = (int)dst->sy/2 - (int)watermark->sy/2;
+                        }else if (ngx_strcmp(conf->watermark_position.data, "center-random") == 0) {
+                            ngx_int_t randomBit = rand() & 1;
+                            if (randomBit) {
+                                wdx = ((int)dst->sx/2 - (int)watermark->sx/2) - (int)((double)rand() / ((double)RAND_MAX + 1) * 15);
+                                wdy = ((int)dst->sy/2 - (int)watermark->sy/2) + (int)((double)rand() / ((double)RAND_MAX + 1) * 15);
+                            } else {
+                                wdx = ((int)dst->sx/2 - (int)watermark->sx/2) + (int)((double)rand() / ((double)RAND_MAX + 1) * 15);
+                                wdy = ((int)dst->sy/2 - (int)watermark->sy/2) - (int)((double)rand() / ((double)RAND_MAX + 1) * 15);
+                            }
+                        }
+
+                        gdImageCopy(watermark_mix, dst, 0, 0, wdx, wdy, watermark->sx, watermark->sy);
+                        gdImageCopy(watermark_mix, watermark, 0, 0, 0, 0, watermark->sx, watermark->sy);
+                        gdImageCopyMerge(dst, watermark_mix, wdx, wdy, 0, 0, watermark->sx, watermark->sy, 75);
+                        gdImageDestroy(watermark_mix);
+
+                    } else {
+                        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "watermark file '%s' is not PNG", conf->watermark.data);
+                    }
+
+                    gdImageDestroy(watermark);
+                } else {
+                    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "watermark file '%s' not found", conf->watermark.data);
+                }
+
+                fclose(watermark_file);
+            }else{
+                if (conf->filter == NGX_HTTP_IMAGE_WATERMARK)
+                {
+                    gdImageDestroy(src);
+                    return ngx_http_image_asis(r, ctx);
+                }
+            }
+        }
+
+
     sharpen = ngx_http_image_filter_get_value(r, conf->shcv, conf->sharpen);
     if (sharpen > 0) {
         gdImageSharpen(dst, sharpen);
@@ -1012,7 +1169,6 @@ transparent:
     b->last_buf = 1;
 
     ngx_http_image_length(r, b);
-    ngx_http_weak_etag(r);
 
     return b;
 }
@@ -1156,7 +1312,6 @@ ngx_http_image_filter_get_value(ngx_http_request_t *r,
     return ngx_http_image_filter_value(&val);
 }
 
-
 static ngx_uint_t
 ngx_http_image_filter_value(ngx_str_t *value)
 {
@@ -1206,6 +1361,9 @@ ngx_http_image_filter_create_conf(ngx_conf_t *cf)
     conf->interlace = NGX_CONF_UNSET;
     conf->buffer_size = NGX_CONF_UNSET_SIZE;
 
+    conf->watermark_width_from = NGX_CONF_UNSET_UINT;
+    conf->watermark_height_from = NGX_CONF_UNSET_UINT;
+
     return conf;
 }
 
@@ -1257,6 +1415,12 @@ ngx_http_image_filter_merge_conf(ngx_conf_t *cf, void *parent, void *child)
     ngx_conf_merge_size_value(conf->buffer_size, prev->buffer_size,
                               1 * 1024 * 1024);
 
+    ngx_conf_merge_str_value(conf->watermark, prev->watermark, NULL);
+    ngx_conf_merge_str_value(conf->watermark_position, prev->watermark_position, "bottom-right");
+    
+    ngx_conf_merge_value(conf->watermark_height_from, prev->watermark_height_from, 0);
+    ngx_conf_merge_value(conf->watermark_width_from, prev->watermark_height_from, 0);
+
     return NGX_CONF_OK;
 }
 
@@ -1285,7 +1449,9 @@ ngx_http_image_filter(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
 
         } else if (ngx_strcmp(value[i].data, "size") == 0) {
             imcf->filter = NGX_HTTP_IMAGE_SIZE;
-
+            
+        } else if (ngx_strcmp(value[i].data, "watermark") == 0) {
+            imcf->filter = NGX_HTTP_IMAGE_WATERMARK;
         } else {
             goto failed;
         }
@@ -1342,7 +1508,8 @@ ngx_http_image_filter(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
 
     } else if (ngx_strcmp(value[i].data, "crop") == 0) {
         imcf->filter = NGX_HTTP_IMAGE_CROP;
-
+    } else if (ngx_strcmp(value[i].data, "watermark") == 0) {
+        imcf->filter = NGX_HTTP_IMAGE_WATERMARK;
     } else {
         goto failed;
     }
