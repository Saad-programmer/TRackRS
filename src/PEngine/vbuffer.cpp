// Creator: Saad AIT YAHIA - @github: Saad-programmer
#include "pengine.h"
#include "vbuffer.h"

#include <utility>
PVBuffer::~PVBuffer(){unload();}
PVBuffer::PVBuffer(PVBuffer&& other) noexcept:buffid(std::exchange(other.buffid,0)),target(other.target){}
PVBuffer& PVBuffer::operator=(PVBuffer&& other) noexcept {
 if(this!=&other){unload();buffid=std::exchange(other.buffid,0);target=other.target;}return *this;
}
void PVBuffer::unload(){if(buffid){glDeleteBuffers(1,&buffid);buffid=0;}}
bool PVBuffer::create(int size,contenttype type,usagemode usage,const void* data){
 unload();target=type==VertexContent?GL_ARRAY_BUFFER:GL_ELEMENT_ARRAY_BUFFER;
 glGenBuffers(1,&buffid);bind();
 const GLenum hint=usage==StreamUsage?GL_STREAM_DRAW:usage==DynamicUsage?GL_DYNAMIC_DRAW:GL_STATIC_DRAW;
 glBufferData(target,size,data,hint);unbind();return true;
}
void PVBuffer::update(int offset,int size,const void* data){bind();glBufferSubData(target,offset,size,data);unbind();}
void PVBuffer::bind(){trackrs::draw::beginBufferUpload();glBindBuffer(target,buffid);}
void PVBuffer::unbind(){glBindBuffer(GL_ARRAY_BUFFER,0);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);}

// RamFile


void PRamFile::write(const void *data, int datasize)
{
    int newextent = cursor + datasize;
    int buffsize = buffer.size();
    if (newextent > buffsize) {
        buffer.resize(newextent);
        if (cursor > buffsize)
            memset(&buffer[buffsize], 0, cursor-buffsize);
    }

    memcpy(&buffer[cursor], data, datasize);
    cursor += datasize;
}




