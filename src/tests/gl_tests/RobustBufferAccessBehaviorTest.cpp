//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RobustBufferAccessBehaviorTest:
//   Various tests related for GL_KHR_robust_buffer_access_behavior.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

#include <array>

using namespace angle;

namespace
{

class RobustBufferAccessBehaviorTest : public ANGLETest
{
  protected:
    RobustBufferAccessBehaviorTest() : mProgram(0), mTestAttrib(-1)
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    void TearDown() override
    {
        glDeleteProgram(mProgram);
        ANGLETest::TearDown();
    }

    bool initExtension()
    {
        EGLWindow *window  = getEGLWindow();
        EGLDisplay display = window->getDisplay();
        if (!eglDisplayExtensionEnabled(display, "EGL_EXT_create_context_robustness"))
        {
            return false;
        }

        ANGLETest::TearDown();
        setRobustAccess(true);
        ANGLETest::SetUp();
        if (!extensionEnabled("GL_KHR_robust_buffer_access_behavior"))
        {
            return false;
        }
        return true;
    }

    void initBasicProgram()
    {
        constexpr char kVS[] =
            "precision mediump float;\n"
            "attribute vec4 position;\n"
            "attribute vec4 vecRandom;\n"
            "varying vec4 v_color;\n"
            "bool testFloatComponent(float component) {\n"
            "    return (component == 0.2 || component == 0.0);\n"
            "}\n"
            "bool testLastFloatComponent(float component) {\n"
            "    return testFloatComponent(component) || component == 1.0;\n"
            "}\n"
            "void main() {\n"
            "    if (testFloatComponent(vecRandom.x) &&\n"
            "        testFloatComponent(vecRandom.y) &&\n"
            "        testFloatComponent(vecRandom.z) &&\n"
            "        testLastFloatComponent(vecRandom.w)) {\n"
            "        v_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
            "    } else {\n"
            "        v_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "    }\n"
            "    gl_Position = position;\n"
            "}\n";

        constexpr char kFS[] =
            "precision mediump float;\n"
            "varying vec4 v_color;\n"
            "void main() {\n"
            "    gl_FragColor = v_color;\n"
            "}\n";

        mProgram = CompileProgram(kVS, kFS);
        ASSERT_NE(0u, mProgram);

        mTestAttrib = glGetAttribLocation(mProgram, "vecRandom");
        ASSERT_NE(-1, mTestAttrib);

        glUseProgram(mProgram);
    }

    void runIndexOutOfRangeTests(GLenum drawType)
    {
        if (mProgram == 0)
        {
            initBasicProgram();
        }

        GLBuffer bufferIncomplete;
        glBindBuffer(GL_ARRAY_BUFFER, bufferIncomplete);
        std::array<GLfloat, 12> randomData = {
            {0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f}};
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * randomData.size(), randomData.data(),
                     drawType);

        glEnableVertexAttribArray(mTestAttrib);
        glVertexAttribPointer(mTestAttrib, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

        glClearColor(0.0, 0.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawIndexedQuad(mProgram, "position", 0.5f);

        int width     = getWindowWidth();
        int height    = getWindowHeight();
        GLenum result = glGetError();
        // For D3D dynamic draw, we still return invalid operation. Once we force the index buffer
        // to clamp any out of range indices instead of invalid operation, this part can be removed.
        // We can always get GL_NO_ERROR.
        if (result == GL_INVALID_OPERATION)
        {
            EXPECT_PIXEL_COLOR_EQ(width * 1 / 4, height * 1 / 4, GLColor::blue);
            EXPECT_PIXEL_COLOR_EQ(width * 1 / 4, height * 3 / 4, GLColor::blue);
            EXPECT_PIXEL_COLOR_EQ(width * 3 / 4, height * 1 / 4, GLColor::blue);
            EXPECT_PIXEL_COLOR_EQ(width * 3 / 4, height * 3 / 4, GLColor::blue);
        }
        else
        {
            EXPECT_GLENUM_EQ(GL_NO_ERROR, result);
            EXPECT_PIXEL_COLOR_EQ(width * 1 / 4, height * 1 / 4, GLColor::green);
            EXPECT_PIXEL_COLOR_EQ(width * 1 / 4, height * 3 / 4, GLColor::green);
            EXPECT_PIXEL_COLOR_EQ(width * 3 / 4, height * 1 / 4, GLColor::green);
            EXPECT_PIXEL_COLOR_EQ(width * 3 / 4, height * 3 / 4, GLColor::green);
        }
    }

    GLuint mProgram;
    GLint mTestAttrib;
};

// Test that static draw with out-of-bounds reads will not read outside of the data store of the
// buffer object and will not result in GL interruption or termination when
// GL_KHR_robust_buffer_access_behavior is supported.
TEST_P(RobustBufferAccessBehaviorTest, DrawElementsIndexOutOfRangeWithStaticDraw)
{
    ANGLE_SKIP_TEST_IF(IsNVIDIA() && IsWindows() && IsOpenGL());

    // Failing after changing the shard count of angle_end2end_tests. http://anglebug.com/2799
    ANGLE_SKIP_TEST_IF(IsNVIDIA() && IsD3D11_FL93());

    ANGLE_SKIP_TEST_IF(!initExtension());

    runIndexOutOfRangeTests(GL_STATIC_DRAW);
}

// Test that dynamic draw with out-of-bounds reads will not read outside of the data store of the
// buffer object and will not result in GL interruption or termination when
// GL_KHR_robust_buffer_access_behavior is supported.
TEST_P(RobustBufferAccessBehaviorTest, DrawElementsIndexOutOfRangeWithDynamicDraw)
{
    ANGLE_SKIP_TEST_IF(IsNVIDIA() && IsWindows() && IsOpenGL());
    ANGLE_SKIP_TEST_IF(!initExtension());

    runIndexOutOfRangeTests(GL_DYNAMIC_DRAW);
}

// Test that vertex buffers are rebound with the correct offsets in subsequent calls in the D3D11
// backend.  http://crbug.com/837002
TEST_P(RobustBufferAccessBehaviorTest, D3D11StateSynchronizationOrderBug)
{
    ANGLE_SKIP_TEST_IF(!initExtension());

    glDisable(GL_DEPTH_TEST);

    // 2 quads, the first one red, the second one green
    const std::array<angle::Vector4, 16> vertices{
        angle::Vector4(-1.0f, 1.0f, 0.5f, 1.0f),   // v0
        angle::Vector4(1.0f, 0.0f, 0.0f, 1.0f),    // c0
        angle::Vector4(-1.0f, -1.0f, 0.5f, 1.0f),  // v1
        angle::Vector4(1.0f, 0.0f, 0.0f, 1.0f),    // c1
        angle::Vector4(1.0f, -1.0f, 0.5f, 1.0f),   // v2
        angle::Vector4(1.0f, 0.0f, 0.0f, 1.0f),    // c2
        angle::Vector4(1.0f, 1.0f, 0.5f, 1.0f),    // v3
        angle::Vector4(1.0f, 0.0f, 0.0f, 1.0f),    // c3

        angle::Vector4(-1.0f, 1.0f, 0.5f, 1.0f),   // v4
        angle::Vector4(0.0f, 1.0f, 0.0f, 1.0f),    // c4
        angle::Vector4(-1.0f, -1.0f, 0.5f, 1.0f),  // v5
        angle::Vector4(0.0f, 1.0f, 0.0f, 1.0f),    // c5
        angle::Vector4(1.0f, -1.0f, 0.5f, 1.0f),   // v6
        angle::Vector4(0.0f, 1.0f, 0.0f, 1.0f),    // c6
        angle::Vector4(1.0f, 1.0f, 0.5f, 1.0f),    // v7
        angle::Vector4(0.0f, 1.0f, 0.0f, 1.0f),    // c7
    };

    GLBuffer vb;
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

    const std::array<GLushort, 12> indicies{
        0, 1, 2, 0, 2, 3,  // quad0
        4, 5, 6, 4, 6, 7,  // quad1
    };

    GLBuffer ib;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies.data(), GL_STATIC_DRAW);

    constexpr char kVS[] = R"(
precision highp float;
attribute vec4 a_position;
attribute vec4 a_color;

varying vec4 v_color;

void main()
{
    gl_Position = a_position;
    v_color = a_color;
})";

    constexpr char kFS[] = R"(
precision highp float;
varying vec4 v_color;

void main()
{
    gl_FragColor = v_color;
})";

    ANGLE_GL_PROGRAM(program, kVS, kFS);
    glUseProgram(program);

    GLint positionLocation = glGetAttribLocation(program, "a_position");
    glEnableVertexAttribArray(positionLocation);
    glVertexAttribPointer(positionLocation, 4, GL_FLOAT, GL_FALSE, sizeof(angle::Vector4) * 2, 0);

    GLint colorLocation = glGetAttribLocation(program, "a_color");
    glEnableVertexAttribArray(colorLocation);
    glVertexAttribPointer(colorLocation, 4, GL_FLOAT, GL_FALSE, sizeof(angle::Vector4) * 2,
                          reinterpret_cast<const void *>(sizeof(angle::Vector4)));

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT,
                   reinterpret_cast<const void *>(sizeof(GLshort) * 6));
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT,
                   reinterpret_cast<const void *>(sizeof(GLshort) * 6));
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);
}

// Covers drawing with a very large vertex range which overflows GLsizei. http://crbug.com/842028
TEST_P(RobustBufferAccessBehaviorTest, VeryLargeVertexCountWithDynamicVertexData)
{
    ANGLE_SKIP_TEST_IF(!initExtension());
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_OES_element_index_uint"));

    constexpr GLsizei kIndexCount           = 32;
    std::array<GLuint, kIndexCount> indices = {{}};
    for (GLsizei index = 0; index < kIndexCount; ++index)
    {
        indices[index] = ((std::numeric_limits<GLuint>::max() - 2) / kIndexCount) * index;
    }

    GLBuffer indexBuffer;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(),
                 GL_STATIC_DRAW);

    std::array<GLfloat, 256> vertexData = {{}};

    GLBuffer vertexBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(GLfloat), vertexData.data(),
                 GL_DYNAMIC_DRAW);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    glUseProgram(program);

    GLint attribLoc = glGetAttribLocation(program, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, attribLoc);

    glVertexAttribPointer(attribLoc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(attribLoc);
    ASSERT_GL_NO_ERROR();

    glDrawElements(GL_TRIANGLES, kIndexCount, GL_UNSIGNED_INT, nullptr);

    // This may or may not generate an error, but it should not crash.
}

// Test that robust access works even if there's no data uploaded to the vertex buffer at all.
TEST_P(RobustBufferAccessBehaviorTest, NoBufferData)
{
    // http://crbug.com/889303: Possible driver bug on NVIDIA Shield TV.
    // http://anglebug.com/2861: Fails abnormally on Pixel XL
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsOpenGLES());

    ANGLE_SKIP_TEST_IF(!initExtension());
    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    glUseProgram(program);

    glEnableVertexAttribArray(0);
    GLBuffer buf;
    glBindBuffer(GL_ARRAY_BUFFER, buf);

    glVertexAttribPointer(0, 1, GL_FLOAT, false, 0, nullptr);
    ASSERT_GL_NO_ERROR();

    std::array<GLubyte, 1u> indices = {0};
    glDrawElements(GL_POINTS, indices.size(), GL_UNSIGNED_BYTE, indices.data());
    ASSERT_GL_NO_ERROR();
}

ANGLE_INSTANTIATE_TEST(RobustBufferAccessBehaviorTest,
                       ES2_D3D9(),
                       ES2_D3D11_FL9_3(),
                       ES2_D3D11(),
                       ES3_D3D11(),
                       ES31_D3D11(),
                       ES2_OPENGL(),
                       ES3_OPENGL(),
                       ES31_OPENGL(),
                       ES2_OPENGLES(),
                       ES3_OPENGLES(),
                       ES31_OPENGLES(),
                       ES2_VULKAN());

}  // namespace
