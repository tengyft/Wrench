/*
 *  Copyright (C) 2000 Tridia Corporation.  All Rights Reserved.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * ultrazip.c - handle ultrazip encoding.
 *
 * This file shouldn't be compiled directly.  It is included multiple times by
 * rfbproto.c, each time with a different definition of the macro BPP.  For
 * each value of BPP, this file defines a function which handles an zlib
 * encoded rectangle with BPP bits per pixel.
 */

#define HandleUltraZipBPP CONCAT2E(HandleUltraZip,BPP)
#define HandleUltraBPP CONCAT2E(HandleUltra,BPP)
#define CARDBPP CONCAT3E(uint,BPP,_t)

static rfbBool
HandleUltraBPP (rfbClient* client, int rx, int ry, int rw, int rh)
{
  rfbZlibHeader hdr;
  int toRead=0;
  int inflateResult=0;
  lzo_uint uncompressedBytes = (( rw * rh ) * ( BPP / 8 ));

  if (!ReadFromRFBServer(client, (char *)&hdr, sz_rfbZlibHeader))
    return false;

  toRead = rfbClientSwap32IfLE(hdr.nBytes);
  if (toRead==0) return true;

  if (uncompressedBytes==0)
  {
      rfbClientLog("ultra error: rectangle has 0 uncomressed bytes ((%dw * %dh) * (%d / 8))\n", rw, rh, BPP); 
      return false;
  }

  /* First make sure we have a large enough raw buffer to hold the
   * decompressed data.  In practice, with a fixed BPP, fixed frame
   * buffer size and the first update containing the entire frame
   * buffer, this buffer allocation should only happen once, on the
   * first update.
   */
  if ( client->raw_buffer_size < (int)uncompressedBytes) {
    if ( client->raw_buffer != NULL ) {
      free( client->raw_buffer );
    }
    client->raw_buffer_size = uncompressedBytes;
    /* buffer needs to be aligned on 4-byte boundaries */
    if ((client->raw_buffer_size % 4)!=0)
      client->raw_buffer_size += (4-(client->raw_buffer_size % 4));
    client->raw_buffer = (char*) malloc( client->raw_buffer_size );
  }
  
  /* allocate enough space to store the incoming compressed packet */
  if ( client->ultra_buffer_size < toRead ) {
    if ( client->ultra_buffer != NULL ) {
      free( client->ultra_buffer );
    }
    client->ultra_buffer_size = toRead;
    /* buffer needs to be aligned on 4-byte boundaries */
    if ((client->ultra_buffer_size % 4)!=0)
      client->ultra_buffer_size += (4-(client->ultra_buffer_size % 4));
    client->ultra_buffer = (char*) malloc( client->ultra_buffer_size );
  }

  /* Fill the buffer, obtaining data from the server. */
  if (!ReadFromRFBServer(client, client->ultra_buffer, toRead))
      return false;

  /* uncompress the data */
  uncompressedBytes = client->raw_buffer_size;
  inflateResult = lzo1x_decompress(
              (lzo_byte *)client->ultra_buffer, toRead,
              (lzo_byte *)client->raw_buffer, (lzo_uintp) &uncompressedBytes,
              NULL);
  
  
  if ((lzo_uint)(rw * rh * (BPP / 8)) != uncompressedBytes)
      rfbClientLog("Ultra decompressed too little (%d < %d)", (rw * rh * (BPP / 8)), uncompressedBytes);
  
  /* Put the uncompressed contents of the update on the screen. */
  if ( inflateResult == LZO_E_OK ) 
  {
    CopyRectangle(client, (unsigned char *)client->raw_buffer, rx, ry, rw, rh);
  }
  else
  {
    rfbClientLog("ultra decompress returned error: %d\n",
            inflateResult);
    return false;
  }
  return true;
}


/* UltraZip is like rre in that it is composed of subrects */
static rfbBool
HandleUltraZipBPP (rfbClient* client, int rx, int ry, int rw, int rh)
{
  rfbZlibHeader hdr;
  int i=0;
  int toRead=0;
  int inflateResult=0;
  unsigned char *ptr=NULL;
  lzo_uint uncompressedBytes = ry + (rw * 65535);
  unsigned int numCacheRects = rx;

  if (!ReadFromRFBServer(client, (char *)&hdr, sz_rfbZlibHeader))
    return false;

  toRead = rfbClientSwap32IfLE(hdr.nBytes);

  if (toRead==0) return true;

  if (uncompressedBytes==0)
  {
      rfbClientLog("ultrazip error: rectangle has 0 uncomressed bytes (%dy + (%dw * 65535)) (%d rectangles)\n", ry, rw, rx); 
      return false;
  }

  /* First make sure we have a large enough raw buffer to hold the
   * decompressed data.  In practice, with a fixed BPP, fixed frame
   * buffer size and the first update containing the entire frame
   * buffer, this buffer allocation should only happen once, on the
   * first update.
   */
  if ( client->raw_buffer_size < (int)(uncompressedBytes + 500)) {
    if ( client->raw_buffer != NULL ) {
      free( client->raw_buffer );
    }
    client->raw_buffer_size = uncompressedBytes + 500;
    /* buffer needs to be aligned on 4-byte boundaries */
    if ((client->raw_buffer_size % 4)!=0)
      client->raw_buffer_size += (4-(client->raw_buffer_size % 4));
    client->raw_buffer = (char*) malloc( client->raw_buffer_size );
  }

 
  /* allocate enough space to store the incoming compressed packet */
  if ( client->ultra_buffer_size < toRead ) {
    if ( client->ultra_buffer != NULL ) {
      free( client->ultra_buffer );
    }
    client->ultra_buffer_size = toRead;
    client->ultra_buffer = (char*) malloc( client->ultra_buffer_size );
  }

  /* Fill the buffer, obtaining data from the server. */
  if (!ReadFromRFBServer(client, client->ultra_buffer, toRead))
      return false;

  /* uncompress the data */
  uncompressedBytes = client->raw_buffer_size;
  inflateResult = lzo1x_decompress(
              (lzo_byte *)client->ultra_buffer, toRead,
              (lzo_byte *)client->raw_buffer, &uncompressedBytes, NULL);
  if ( inflateResult != LZO_E_OK ) 
  {
    rfbClientLog("ultra decompress returned error: %d\n",
            inflateResult);
    return false;
  }
  
  /* Put the uncompressed contents of the update on the screen. */
  ptr = (unsigned char *)client->raw_buffer;
  for (i=0; i<(int)numCacheRects; i++)
  {
    unsigned short sx, sy, sw, sh;
    unsigned int se;

    memcpy((char *)&sx, ptr, 2); ptr += 2;
    memcpy((char *)&sy, ptr, 2); ptr += 2;
    memcpy((char *)&sw, ptr, 2); ptr += 2;
    memcpy((char *)&sh, ptr, 2); ptr += 2;
    memcpy((char *)&se, ptr, 4); ptr += 4;

    sx = rfbClientSwap16IfLE(sx);
    sy = rfbClientSwap16IfLE(sy);
    sw = rfbClientSwap16IfLE(sw);
    sh = rfbClientSwap16IfLE(sh);
    se = rfbClientSwap32IfLE(se);

    if (se == rfbEncodingRaw)
    {
        CopyRectangle(client, (unsigned char *)ptr, sx, sy, sw, sh);
        ptr += ((sw * sh) * (BPP / 8));
    }
  }  

  return true;
}

#undef CARDBPP
