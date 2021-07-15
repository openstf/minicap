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

import android.graphics.Rect
import android.os.IBinder
import android.view.Surface
import org.slf4j.Logger
import org.slf4j.LoggerFactory

/**
 * Provide access to the SurfaceControl which is not part of the Android SDK using reflection.
 * This SurfaceControl relies on a jni bindings that manages the SurfaceComposerClient that is
 * in use in minicap-shared library.
 */
object SurfaceControl {

    private val log: Logger = LoggerFactory.getLogger(SurfaceControl::class.java.simpleName)
    private var clazz: Class<*>? = null

    init {
        try {
            clazz = Class.forName("android.view.SurfaceControl")
        } catch (e: ClassNotFoundException) {
            log.error("could not instantiate SurfaceControl", e)
            throw Error(e)
        }
    }

    private fun logAndThrow(e: Exception) {
        log.error("SurfaceControl error", e)
        throw Error(e)
    }

    fun openTransaction() = try {
        clazz!!.getMethod("openTransaction").invoke(null)
    } catch (e: Exception) {
        logAndThrow(e)
    }

    fun closeTransaction() = try {
        clazz!!.getMethod("closeTransaction").invoke(null)
    } catch (e: Exception) {
        logAndThrow(e)
    }

    fun setDisplaySurface(display: IBinder?, consumer: Surface?) = try {
        clazz!!.getMethod("setDisplaySurface", IBinder::class.java, Surface::class.java)
            .invoke(null, display, consumer)
    } catch (e: Exception) {
        logAndThrow(e)
    }

    fun setDisplayLayerStack(displayToken: IBinder?, layerStack: Int) = try {
        clazz!!.getMethod("setDisplayLayerStack", IBinder::class.java, Int::class.javaPrimitiveType)
            .invoke(null, displayToken, layerStack)
    } catch (e: Exception) {
        logAndThrow(e)
    }

    fun setDisplayProjection(
        displayToken: IBinder?,
        rotation: Int, //Rotation0 = 0, Rotation90 = 1, Rotation180 = 2, Rotation270 = 3
        layerStackRect: Rect?,
        displayRect: Rect?
    ) {
        try {
            clazz!!.getMethod(
                "setDisplayProjection", IBinder::class.java,
                Int::class.javaPrimitiveType, Rect::class.java, Rect::class.java
            )
                .invoke(null, displayToken, rotation, layerStackRect, displayRect)
        } catch (e: Exception) {
            logAndThrow(e)
        }
    }

    fun createDisplay(name: String?, secure: Boolean): IBinder {
        try {
            return clazz!!.getMethod(
                "createDisplay",
                String::class.java,
                Boolean::class.javaPrimitiveType
            ).invoke(null, name, secure) as IBinder
        } catch (e: Exception) {
            log.error("SurfaceControl error", e)
            throw Error(e)
        }
    }
}
