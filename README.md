# WinFontRender

This is a small, easy to use, single-header C++ library that renders Windows fonts in graphics applications.

## Problem

While developing various kinds of programs, we often take the possibility of displaying text for granted, as we get this functionality out-of-the-box. This is the case in console apps, where we can just print characters and they are displayed in the terminal. This is also the case in GUI apps, whether made in Windows Forms, WPF, Qt, wxWidgets, MFC - we have labels, buttons, and other controls available.

However, this is not the case when we develop a graphics program or a game using one of the graphics API like Direct3D, OpenGL, or Vulkan. Then the only thing we can do is rendering triangles covered with textures. That's how graphics cards work after all. Displaying text is challenging in this environment, as every single character has to be turned into a textured quad, made of two triangles.

## Solution

This library provides solution to this problem by implementing a `CFont` class. It does two things:

1. It renders characters of the font to a texture.

   ![Font texture](README_files/FontTexture.png "Font texture")

2. It calculates vertices needed to render given text.
