#include <exception.h>
#include <MemManager.h>
#include <GraphicsVideo.h>

GraphicsVideo* GraphicsVideo::_instance = nullptr;

void GraphicsVideo::Create()
{
  static bool _done = false;
  //can be called only once
  if(_done)
    throw upan::exception(XLOC, "cannot create GraphicsVideo driver!");
  _done = true;
  auto f = MultiBoot::Instance().VideoFrameBufferInfo();
  if(f)
  {
    static GraphicsVideo v(*f);
    _instance = &v;
  }
}

GraphicsVideo::GraphicsVideo(const framebuffer_info_t& fbinfo)
{
  _lfbaddress = fbinfo.framebuffer_addr;
  _height = fbinfo.framebuffer_height;
  _width = fbinfo.framebuffer_width;
  _pitch = fbinfo.framebuffer_pitch;
  _bpp = fbinfo.framebuffer_bpp;
  _bytesPerPixel = _bpp / 8;
}

void GraphicsVideo::MemMapLFB()
{
  unsigned uiPDEAddress = MEM_PDBR;
  unsigned noOfPages = ((_height * _width * 4) / PAGE_SIZE) + 1;
  for(unsigned i = 0; i < noOfPages; ++i)
  {
    unsigned addr = _lfbaddress + PAGE_SIZE * i;
    unsigned uiPDEIndex = ((_lfbaddress >> 22) & 0x3FF);
    unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000;
    unsigned uiPTEIndex = ((addr >> 12) & 0x3FF);
    // This page is a Read Only area for user process. 0x5 => 101 => User Domain, Read Only, Present Bit
    ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = (addr & 0xFFFFF000) | 0x5;
    MemManager::Instance().MarkPageAsAllocated(addr / PAGE_SIZE);
  }
  Mem_FlushTLB();

  //FillRect(0, 0, _width, _height, 0xAAAA00);
  DrawChar(100, 100);
}

void GraphicsVideo::SetPixel(unsigned x, unsigned y, unsigned color)
{
  if(y >= _height || x >= _width)
    return;
  unsigned* p = (unsigned*)(_lfbaddress + y * _pitch + x * _bytesPerPixel);
  *p = (color | 0xFF000000);
}

void GraphicsVideo::FillRect(unsigned sx, unsigned sy, unsigned width, unsigned height, unsigned color)
{
  unsigned y_offset;
  for(unsigned y = sy; y < (sy + height) && y < _height; ++y)
  {
    y_offset = y * _pitch;
    for(unsigned x = sx; x < (sx + width) && x < _width; ++x)
    {
      unsigned* p = (unsigned*)(_lfbaddress + y_offset + x * _bytesPerPixel);
      *p = (color | 0xFF000000);
    }
  }
}

byte font_data[] = {
//  0x18,
//  0x3C,
//  0x66,
//  0x7E,
//  0x66,
//  0x66,
//  0x00,
//  0x00
		0x00, /* 00000000 */
		0x00, /* 00000000 */
		0x7c, /* 01111100 */
		0x06, /* 00000110 */
		0x7e, /* 01111110 */
		0xc6, /* 11000110 */
		0x7e, /* 01111110 */
		0x00, /* 00000000 */
};

void GraphicsVideo::DrawChar(unsigned x, unsigned y)
{
  for(unsigned f = 0; f < 8; ++f, ++y)
  {
    unsigned xa = x;
    for(unsigned i = 8; i > 0; --i, ++xa)
    {
      if(font_data[f] & (1 << i))
        SetPixel(xa, y, 0xFFFFFF);
      else
        SetPixel(xa, y, 0);
    }
  }
}
