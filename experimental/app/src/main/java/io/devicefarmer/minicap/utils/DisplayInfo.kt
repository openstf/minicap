/*
 * Copyright (C) 2020 Orange
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package io.devicefarmer.minicap.utils

import android.util.Size

data class DisplayInfo(
    val displayId: Int,
    val size: Size,
    val rotation: Int,
    val layerStack: Int,
    val density: Int,
    val xdpi: Float,
    val ydpi: Float
) {
    override fun toString(): String = "{\n" +
            "   \"id\": ${displayId},\n" +
            "   \"width\": ${size.width},\n" +
            "   \"height\": ${size.height},\n" +
            "   \"xdpi\": ${xdpi},\n" +
            "   \"ydpi\": ${ydpi},\n" +
            "   \"density\": ${density},\n" +
            "   \"rotation\": ${rotation}\n" +
            "}\n"
}
