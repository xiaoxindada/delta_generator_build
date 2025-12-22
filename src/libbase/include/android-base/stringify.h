/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

// Converts macro argument 'x' into a string constant without macro expansion.
// So QUOTE(EINVAL) would be "EINVAL".
#define QUOTE(x...) #x

// Converts macro argument 'x' into a string constant after macro expansion.
// So STRINGIFY(EINVAL) would be "22".
#define STRINGIFY(x...) QUOTE(x)
