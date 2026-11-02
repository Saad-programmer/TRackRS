// Creator: Saad AIT YAHIA - @github: Saad-programmer
#pragma once

#include "vmath.h"
#include <glad/gl.h>

#define BUFFER_OFFSET(i) ((char *)nullptr + (i))


class PVBuffer {
public:
    enum contenttype {
        VertexContent, IndexContent
    };
    enum usagemode {
        StreamUsage, StaticUsage, DynamicUsage
    };

private:
    GLuint buffid=0;
    GLenum target=GL_ARRAY_BUFFER;
public:
    PVBuffer() = default;
    ~PVBuffer();
    PVBuffer(const PVBuffer&) = delete;
    PVBuffer& operator=(const PVBuffer&) = delete;
    PVBuffer(PVBuffer&& other) noexcept;
    PVBuffer& operator=(PVBuffer&& other) noexcept;
    bool create(int size, contenttype type, usagemode usage, const void* data=nullptr);
    void update(int offset,int size,const void* data);
    void unload();
    GLuint id() const { return buffid; }
    void bind();
    static void unbind();
};


class PRamFile {
protected:
    std::vector<uint8> buffer;
    int cursor;

public:
    PRamFile() { cursor = 0; }

    void write(const void *data, int datasize);

    void seek_cur(int pos) { seek_set(pos + cursor); }
    void seek_end(int pos) { seek_set(pos + buffer.size()); }
    void seek_set(int pos) { cursor = (pos >= 0) ? pos : 0; }
    int tell() { return cursor; }

    void clear() { buffer.clear(); cursor = 0; }

    void *getData() { return &buffer[0]; }
    int getSize() { return buffer.size(); }
};



