/*
 * Copyright (C) 2020 Huawei Technologies Co., Ltd.
 *   @Author: Julian Bouzas <julian.bouzas@collabora.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __GST_AMS_ELEMENTS_H__
#define __GST_AMS_ELEMENTS_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_AMS_ALIGN(_DATA_, _ALIGN_)     (((_DATA_)+(_ALIGN_-1))&~(_ALIGN_-1))

#define  AMS_DEBUG_LOG
#ifdef   AMS_DEBUG_LOG
#define GST_AMS_LOG(FMT, ...) printf("[%s:%s:%d]"FMT"\r\n", __FILE__,__FUNCTION__,__LINE__, ##__VA_ARGS__)
#else
#define GST_AMS_LOG(FMT, ...)
#endif

GST_ELEMENT_REGISTER_DECLARE (amsh264dec);

GST_ELEMENT_REGISTER_DECLARE (amsh264enc);

GST_ELEMENT_REGISTER_DECLARE (amsh265dec);

GST_ELEMENT_REGISTER_DECLARE (amsh265enc);

G_END_DECLS

#endif /* __GST_AMS_ELEMENTS_H__ */
