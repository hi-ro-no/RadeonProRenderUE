/**********************************************************************
* Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
********************************************************************/

#include "MaterialGraph/Node/Factory/RPRMaterialGLTFNodeTypeParser.h"

TMap<GLTF::ERPRNodeType, RPRMaterialGLTF::ERPRMaterialNodeType> FRPRMaterialGLTFNodeTypeParser::GLTFTypeEnumToUETypeEnumMap;

void FRPRMaterialGLTFNodeTypeParser::InitializeParserMapping()
{
    GLTFTypeEnumToUETypeEnumMap.Add(GLTF::ERPRNodeType::UBER, RPRMaterialGLTF::ERPRMaterialNodeType::Uber);
    GLTFTypeEnumToUETypeEnumMap.Add(GLTF::ERPRNodeType::NORMAL_MAP, RPRMaterialGLTF::ERPRMaterialNodeType::NormalMap);
    GLTFTypeEnumToUETypeEnumMap.Add(GLTF::ERPRNodeType::IMAGE_TEXTURE, RPRMaterialGLTF::ERPRMaterialNodeType::ImageTexture);
}

RPRMaterialGLTF::ERPRMaterialNodeType FRPRMaterialGLTFNodeTypeParser::ParseTypeFromGLTF(const GLTF::FRPRNode& Node)
{
    if (GLTFTypeEnumToUETypeEnumMap.Num() == 0)
    {
        InitializeParserMapping();
    }

    const RPRMaterialGLTF::ERPRMaterialNodeType* NodeType = GLTFTypeEnumToUETypeEnumMap.Find(Node.type);
    if (NodeType != nullptr)
    {
        return *NodeType;
    }

    return RPRMaterialGLTF::ERPRMaterialNodeType::Unsupported;
}