#pragma once

#include <SkBitmap.h>
#include <SkDevice.h>
#include <SkPaint.h>
#include <SkRect.h>
#include <SkData.h>
#include <SkImage.h>
#include <SkStream.h>
#include <SkSurface.h>
#include <SkPath.h>
#include <SkTypeface.h>
#include "controlmanager.h"

void RenderControlsToBitmap(PGRendererHandle renderer, SkBitmap& bitmap, PGIRect rect, ControlManager* manager);
PGRendererHandle InitializeRenderer();
SkBitmap* PGGetBitmap(PGBitmapHandle);