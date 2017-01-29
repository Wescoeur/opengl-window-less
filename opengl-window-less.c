/*
 * Copyright 2017 Ronan Abhamon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifdef __linux__
  #include <stdlib.h>
  #include <GL/gl.h>
  #include <GL/glx.h>
#elif _WIN32
  #include <stdlib.h>
  #include <malloc.h>
  #include <Wingdi.h>
  #include <Winuser.h>
#else
  #error "platform not supported"
#endif

#include "opengl-window-less.h"

// =============================================================================

#ifdef __linux__

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, Bool, const int *);
typedef Bool (*glXMakeContextCurrentARBProc)(Display *, GLXDrawable, GLXDrawable, GLXContext);

struct OwlContext {
  Display *display;
  GLXContext context;
  GLXPbuffer pbuffer;
};

OwlContext *owl_create_context (void) {
  static const int fb_config_attribs[] = { None };
  static const int context_attribs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 0,
    None
  };
  static const int pbuffer_attribs[] = {
    GLX_PBUFFER_WIDTH, 640,
    GLX_PBUFFER_HEIGHT, 480,
    None
  };

  OwlContext *owl_context = NULL;
  Display *display = NULL;

  GLXFBConfig *fb_config = NULL;
  GLXContext context = NULL;
  GLXPbuffer pbuffer = 0;

  int nitems;

  glXCreateContextAttribsARBProc glXCreateContextAttribsARB;
  glXMakeContextCurrentARBProc glXMakeContextCurrentARB;

  // Init vars to build context.
  if (
    !(display = XOpenDisplay(NULL)) ||
    !(fb_config = glXChooseFBConfig(display, DefaultScreen(display), fb_config_attribs, &nitems)) ||
    !(glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((GLubyte *)"glXCreateContextAttribsARB")) ||
    !(glXMakeContextCurrentARB = (glXMakeContextCurrentARBProc)glXGetProcAddressARB((GLubyte *)"glXMakeContextCurrent")) ||
    !(context = glXCreateContextAttribsARB(display, fb_config[0], 0, True, context_attribs)) ||
    !(pbuffer = glXCreatePbuffer(display, fb_config[0], pbuffer_attribs))
  )
    goto error;

  XFree(fb_config);
  fb_config = NULL;

  XSync(display, False);

  // Bind context.
  if (!glXMakeContextCurrent(display, pbuffer, pbuffer, context)) {
    glXDestroyPbuffer(display, pbuffer);
    pbuffer = 0;

    if (!glXMakeContextCurrent(display, DefaultRootWindow(display), DefaultRootWindow(display), context))
      goto error;
  }

  // Create owl context structure.
  if (!(owl_context = malloc(sizeof *owl_context)))
    goto error;

  owl_context->display = display;
  owl_context->context = context;
  owl_context->pbuffer = pbuffer;

  return owl_context;

 error:

  if (pbuffer)
    glXDestroyPbuffer(display, pbuffer);

  if (context)
    glXDestroyContext(display, context);

  if (fb_config)
    XFree(fb_config);

  if (display)
    XCloseDisplay(display);

  return NULL;
}

void owl_destroy_context (OwlContext *context) {
  if (context == NULL)
    return;

  if (context->context == glXGetCurrentContext())
    glXMakeContextCurrent(context->display, None, None, NULL);

  if (context->pbuffer)
    glXDestroyPbuffer(context->display, context->pbuffer);

  glXDestroyContext(context->display, context->context);
  XCloseDisplay(context->display);

  free(context);

  return;
}

#endif

// -----------------------------------------------------------------------------

#ifdef _WIN32

struct OwlContext {
  HDC hdc;
  HGLRC context;
};

OwlContext *owl_create_context (void) {
  static PIXELFORMATDESCRIPTOR pfd = {
    .nSize = sizeof(PIXELFORMATDESCRIPTOR),
    .nVersion = 1,
    .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
    .iPixelType = PFD_TYPE_RGBA,
    .iLayerType = PFD_MAIN_PLANE
  };

  OwlContext *owl_context = NULL;

  HDC hdc = NULL;
  int pixel_format;
  HGLRC context = NULL;

  if (
    !(hdc = GetDC(NULL)) ||
    !(pixel_format = ChoosePixelFormat(hdc, &pfd)) ||
    !SetPixelFormat(hdc, pixel_format, &pfd) ||
    !(context = wglCreateContext(hdc)) ||
    !wglMakeCurrent(hdc, context) ||
    !(owl_context = malloc(sizeof *owl_context))
  )
    goto error;

  owl_context->hdc = hdc;
  owl_context->context = context;

  return owl_context;

 error:

  if (context)
    wglDeleteContext​(context);

  if (hdc)
    ReleaseDC(NULL, hdc);

  return NULL;
}

void owl_destroy_context (OwlContext *context) {
  if (context == NULL)
    return;

  if (context->context == wglGetCurrentContext())
    wglMakeCurrent(context->hdc, NULL);

  wglDeleteContext​(context->context);
  ReleaseDC(NULL, context->hdc);

  return;
}

#endif
