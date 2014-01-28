#version 440

// Note 'location'-only tests for enhanced layouts are in 330.frag
// Generic 'component' tests are in 440.vert

// a consumes components 2 and 3 of location 4
layout(location = 4, component = 2) in vec2 a; 

// b consumes component 1 of location 4
layout(location = 4, component = 1) in float b; 
layout(location = 4, component = 2) in vec2 h;  // ERROR, component overlap not okay for fragment in

layout(location = 3, component = 2) in vec3 c;  // ERROR: c overflows components 2 and 3

// e consumes beginning (components 0, 1 and 2) of each of 6 slots
layout(location = 20, component = 0) in vec3 e[6];

// f consumes last component of the same 6 slots 
layout(location = 20, component = 3) in float f[6];

layout(location = 30, component = 3) out int be;
layout(location = 30, component = 0) out vec3 bf;  // ERROR, not the same basic type

writeonly uniform;          // ERROR
readonly in;                // ERROR
flat out;                   // ERROR
mediump uniform;

layout(offset=12) uniform;  // ERROR
layout(offset=12) in;       // ERROR
layout(offset=12) out;      // ERROR

layout(align=16) uniform;   // ERROR
layout(align=16) in;        // ERROR
layout(align=16) out;       // ERROR

layout(offset=12) uniform  ubl1 { int a; } inst1;  // ERROR
layout(offset=12)      in inbl2 { int a; } inst2;  // ERROR
layout(offset=12)     out inbl3 { int a; } inst3;  // ERROR

layout(align=16, std140) uniform  ubl4 { int a; } inst4;
layout(align=16) uniform  ubl8 { int a; } inst8;  // ERROR, no packing
layout(align=16)      in inbl5 { int a; } inst5;  // ERROR
layout(align=16)     out inbl6 { int a; } inst6;  // ERROR

layout(offset=12) uniform vec4 v1;  // ERROR
layout(offset=12)      in vec4 v2;  // ERROR
layout(offset=12)     out vec4 v3;  // ERROR

layout(align=16) uniform vec4 v4;   // ERROR
layout(align=16)      in vec4 v5;   // ERROR
layout(align=16)     out vec4 v6;   // ERROR

layout(std140) in;                  // ERROR
layout(std140) uniform vec4 v7;     // ERROR

layout(align=48) uniform ubl7 {          // ERROR, not power of 2
    layout(offset=12, align=4) float f;  // ERROR, no packing
} inst7;

in ibl10 {
    layout(offset=12) float f;  // ERROR
    layout(align=4) float g;    // ERROR
} inst10;

layout(std430) uniform;

layout(align=32) uniform ubl9 {
    float e;
    layout(offset=12, align=4) float f;
    layout(offset=20) float g;
    float h;
} inst9;

uniform ubl11 {
    layout(offset=12, align=4) float f;
    float g;
} inst11;

layout(std140) uniform block {
                        vec4   a;     // a takes offsets 0-15
    layout(offset = 20) vec3   b;     // b takes offsets 32-43
    layout(offset = 40) vec2   c;     // ERROR, lies within previous member
    layout(offset = 48) vec2   d;     // d takes offsets 48-55
    layout(align = 16)  float  e;     // e takes offsets 64-67
    layout(align = 2)   double f;     // f takes offsets 72-79
    layout(align = 6)   double g;     // ERROR, 6 is not a power of 2
    layout(offset = 80) float  h;     // h takes offsets 80-83
    layout(align = 64)  dvec3  i;     // i takes offsets 128-151
    layout(offset = 153, align = 8) float  j;     // j takes offsets 160-163
} specExample;
