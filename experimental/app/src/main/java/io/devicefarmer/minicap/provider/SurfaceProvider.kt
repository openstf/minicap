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

package io.devicefarmer.minicap.provider

import android.graphics.Rect
import android.media.ImageReader
import android.net.LocalSocket
import android.os.Build
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.util.Size
import io.devicefarmer.minicap.output.ScreenshotOutput
import io.devicefarmer.minicap.utils.DisplayInfo
import io.devicefarmer.minicap.utils.DisplayManagerGlobal
import io.devicefarmer.minicap.utils.SurfaceControl
import java.io.PrintStream
import kotlin.system.exitProcess

/**
 * Provides screen images using [SurfaceControl]. This is pretty similar to the native version
 * of minicap but here it is done at a higher level making things a bit easier.
 */
class SurfaceProvider(displayId: Int, targetSize: Size, orientation: Int) : BaseProvider(displayId, targetSize, orientation) {
    constructor(display: Int) : this(display, currentScreenSize(), currentRotation())

    companion object {
        private fun currentScreenSize(): Size {
            return currentDisplayInfo().run {
                Size(this.size.width, this.size.height)
            }
        }

        private fun currentRotation(): Int = currentDisplayInfo().rotation

        private fun currentDisplayInfo(): DisplayInfo {
            return DisplayManagerGlobal.getDisplayInfo(0)
        }
    }

    private val handler: Handler = Handler(Looper.getMainLooper())
    private var display: IBinder? = null

    val displayInfo: DisplayInfo = DisplayManagerGlobal.getDisplayInfo(displayId)

    override fun getScreenSize(): Size = displayInfo.size


    override fun screenshot(printer: PrintStream) {
        init(ScreenshotOutput(printer))
        initSurface {
            super.onImageAvailable(it)
            exitProcess(0)
        }
    }

    /**
     *
     */
    override fun onConnection(socket: LocalSocket) {
        super.onConnection(socket)
        initSurface()
    }

    /**
     * Setup the Surface between the display and an ImageReader so that we can grab the
     * screen.
     */
    private fun initSurface(l: ImageReader.OnImageAvailableListener) {
        //must be done on the main thread
        // Support  Android 12 (preview),and resolve black screen problem
        val secure =
            Build.VERSION.SDK_INT < Build.VERSION_CODES.R || Build.VERSION.SDK_INT == Build.VERSION_CODES.R && "S" != Build.VERSION.CODENAME
        display = SurfaceControl.createDisplay("minicap", secure)
        //initialise the surface to get the display in the ImageReader
        SurfaceControl.openTransaction()
        try {
            SurfaceControl.setDisplaySurface(display, getImageReader().surface)
            SurfaceControl.setDisplayProjection(
                display,
                0,
                Rect(0, 0, getScreenSize().width, getScreenSize().height),
                Rect(0, 0, getTargetSize().width, getTargetSize().height)
            )
            SurfaceControl.setDisplayLayerStack(display, displayInfo.layerStack)
        } finally {
            SurfaceControl.closeTransaction()
        }
        getImageReader().setOnImageAvailableListener(l, handler)
    }

    private fun initSurface() {
        initSurface(this)
    }
}
