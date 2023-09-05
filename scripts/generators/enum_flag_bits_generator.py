#!/usr/bin/python3 -i
#
# Copyright (c) 2015-2023 The Khronos Group Inc.
# Copyright (c) 2015-2023 Valve Corporation
# Copyright (c) 2015-2023 LunarG, Inc.
# Copyright (c) 2015-2023 Google Inc.
# Copyright (c) 2023-2023 RasterGrid Kft.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
from generators.base_generator import BaseGenerator

# This class is a container for any source code, data, or other behavior that is necessary to
# customize the generator script for a specific target API variant (e.g. Vulkan SC). As such,
# all of these API-specific interfaces and their use in the generator script are part of the
# contract between this repository and its downstream users. Changing or removing any of these
# interfaces or their use in the generator script will have downstream effects and thus
# should be avoided unless absolutely necessary.
class APISpecific:
    # Generates manual constants
    @staticmethod
    def genManualConstants(targetApiName: str) -> list[str]:
        match targetApiName:

            # Vulkan specific manual constant generation
            case 'vulkan':
                return []


class EnumFlagBitsOutputGenerator(BaseGenerator):
    def __init__(self):
        BaseGenerator.__init__(self)

    def generate(self):

        bitmasks = [] # List[Bitmask]
        # Build up a list of which flagBits are actually called
        # Some flagBits are only for return values, so the user will never call a function
        # using them, and can reduce which ones we generate
        for struct in [x for x in self.vk.structs.values() if not x.returnedOnly]:
            for member in struct.members:
                flagName = member.type.replace('Flags', 'FlagBits')
                if member.type in self.vk.bitmasks and self.vk.bitmasks[member.type] not in bitmasks:
                    bitmasks.append(self.vk.bitmasks[member.type])
                elif flagName in self.vk.bitmasks and self.vk.bitmasks[flagName] not in bitmasks:
                    bitmasks.append(self.vk.bitmasks[flagName])
        for command in self.vk.commands.values():
            for param in command.params:
                flagName = param.type.replace('Flags', 'FlagBits')
                if param.type in self.vk.bitmasks and self.vk.bitmasks[param.type] not in bitmasks:
                    bitmasks.append(self.vk.bitmasks[param.type])
                elif flagName in self.vk.bitmasks and self.vk.bitmasks[flagName] not in bitmasks:
                    bitmasks.append(self.vk.bitmasks[flagName])

        out = [
            f'''// *** THIS FILE IS GENERATED - DO NOT EDIT ***
// See {os.path.basename(__file__)} for modifications

/***************************************************************************
*
* Copyright (c) 2015-2023 The Khronos Group Inc.
* Copyright (c) 2015-2023 Valve Corporation
* Copyright (c) 2015-2023 LunarG, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
****************************************************************************/\n''',
            '// NOLINTBEGIN',
            '''
#pragma once

#include <array>
#include "vulkan/vulkan.h"\n''',
        ]
        out.append(f'const uint32_t GeneratedVulkanHeaderVersion = {self.vk.headerVersion};\n')
        for bitmask in bitmasks:
            if (
                bitmask.flagName == 'VkGeometryInstanceFlagsKHR'
                or len(bitmask.flags) == 0
            ):
                continue # only called in VkAccelerationStructureInstanceKHR which is never called anywhere explicitly
            out.extend([f'#ifdef {bitmask.protect}\n'] if bitmask.protect else [])
            out.extend(
                (
                    f'const {bitmask.flagName} All{bitmask.name} = {"|".join([flag.name for flag in bitmask.flags])}',
                    ';\n',
                )
            )
            out.extend([f'#endif //{bitmask.protect}\n'] if bitmask.protect else [])

        out.extend(APISpecific.genManualConstants(self.targetApiName))

        out.extend(
            (
                '\n',
                '// mask of all the VK_PIPELINE_STAGE_*_SHADER_BIT stages\n',
                f'const VkPipelineStageFlagBits2 allVkPipelineShaderStageBits2 = {"|".join([flag.name for flag in self.vk.bitmasks["VkPipelineStageFlagBits2"].flags if "_SHADER_BIT" in flag.name])};\n',
                '\n',
            )
        )
        flagBitsAsArray = ['VkQueueFlagBits', 'VkShaderStageFlagBits']
        out.extend(
            f'[[maybe_unused]] constexpr std::array All{bitmask.flagName} = {{{",".join([flag.name for flag in bitmask.flags])}}};\n'
            for bitmask in [self.vk.bitmasks[x] for x in flagBitsAsArray]
        )
        out.append('// NOLINTEND') # Wrap for clang-tidy to ignore
        self.write("".join(out))
