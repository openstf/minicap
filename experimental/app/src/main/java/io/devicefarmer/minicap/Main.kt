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

package io.devicefarmer.minicap

import android.os.Looper
import android.util.Size
import io.devicefarmer.minicap.provider.SurfaceProvider
import kotlin.math.roundToInt
import kotlin.system.exitProcess


/**
 * Main entry point that can be launched as follow:
 * adb shell CLASSPATH=/data/local/tmp/minicap.apk app_process /system/bin io.devicefarmer.minicap.Main
 */
class Main {
    companion object {

        @JvmStatic
        fun main(args: Array<String>) {
            val provider: SurfaceProvider
            //called because we are not run from the Android environment
            Looper.prepareMainLooper()

            val params = args.fold(Pair(Parameters.Builder(), "")) { (p, lastKey), elem ->
                if (elem.startsWith("-")) {
                    when (elem) {
                        "-s" -> p.screenshot(true)
                        "-i" -> p.displayInfo(true)
                        "-h" -> showHelp().also { System.exit(0) }
                    }
                    Pair(p, elem)
                } else {
                    when (lastKey) {
                        "-d" -> p.displayId(elem.toInt())
                        "-n" -> p.socket(elem)
                        "-P" -> p.projection(elem)
                        "-Q" -> p.quality(elem.toInt())
                        "-r" -> p.frameRate(elem.toFloat())
                    }
                    Pair(p, lastKey)
                }
            }.first.build()

            provider = if (params.projection == null) {
                SurfaceProvider(params.displayId)
            } else {
                params.projection.forceAspectRatio()
                SurfaceProvider(
                    params.displayId,
                    Size(
                        params.projection.targetSize.width,
                        params.projection.targetSize.height
                    ),
                    angleToRotation(params.projection.rotation)
                )
            }
            provider.quality = params.quality
            provider.frameRate = params.frameRate
            when {
                params.displayInfo -> {
                    println("${provider.displayInfo}")
                    exitProcess(0)
                }
                params.screenshot -> {
                    //for now this is asynchronous and requires the main looper to run
                    provider.screenshot(System.out)
                }
                else -> {
                    //the stf process reads this
                    System.err.println("PID: ${android.os.Process.myPid()}")
                    System.err.println("INFO: ${params.projection}")
                    val server = SimpleServer(params.socket, provider)
                    server.start()
                }
            }
            Looper.loop()
        }

        private fun showHelp() {
            System.out.println(
                "Usage: %s [-h] [-n <name>]\n" +
                        "  -d <id>:       Display ID. (%d)\n" +
                        "  -n <name>:     Change the name of the abtract unix domain socket. (%s)\n" +
                        "  -P <value>:    Display projection (<w>x<h>@<w>x<h>/{0|90|180|270}).\n" +
                        "  -Q <value>:    JPEG quality (0-100).\n" +
                        "  -s:            Take a screenshot and output it to stdout. Needs -P.\n" +
                        "  -S:            Skip frames when they cannot be consumed quickly enough.\n" +
                        "  -r <value>:    Frame rate (frames/s)" +
                        "  -t:            Attempt to get the capture method running, then exit.\n" +
                        "  -i:            Get display information in JSON format. May segfault.\n" +
                        "  -h:            Show help.\n"
            )
        }
    }
}

private fun angleToRotation(value: Int): Int =
    when(value) {
        0   -> 0
        90  -> 1
        180 -> 2
        270 -> 3
        else -> throw IllegalStateException("Invalid rotation")
    }

data class Projection(
    val realSize: Size, var targetSize: Size,
    val rotation: Int
) {
    fun forceAspectRatio() {
        val aspect = realSize.width.toFloat() / realSize.height.toFloat()
        targetSize = if (targetSize.height > targetSize.width / aspect) {
            Size(targetSize.width, ((targetSize.width / aspect)).roundToInt())
        } else {
            Size((targetSize.height * aspect).roundToInt(), targetSize.height)
        }
    }

    override fun toString(): String =
        "${realSize.width}x${realSize.height}@${targetSize.width}x${targetSize.height}/${rotation}"
}

class Parameters private constructor(
    val projection: Projection?,
    val screenshot: Boolean,
    val socket: String,
    val quality: Int,
    val displayInfo: Boolean,
    val frameRate: Float,
    val displayId: Int
) {
    data class Builder(
        var projection: Projection? = null,
        var screenshot: Boolean = false,
        var socket: String = "minicap",
        var quality: Int = 100,
        var displayInfo: Boolean = false,
        var frameRate: Float = Float.MAX_VALUE,
        var displayId: Int = 0
    ) {
        //TODO make something more robust
        fun projection(p: String) = apply {
            this.projection = p.run {
                val s = this.split('@', '/', 'x')
                Projection(
                    Size(s[0].toInt(), s[1].toInt()),
                    Size(s[2].toInt(), s[3].toInt()),
                    s[4].toInt()
                )
            }
        }

        fun screenshot(s: Boolean) = apply { this.screenshot = s }
        fun socket(name: String) = apply { this.socket = name }
        fun quality(value: Int) = apply { this.quality = value }
        fun displayInfo(enabled: Boolean) = apply { this.displayInfo = enabled }
        fun frameRate(value: Float) = apply { this.frameRate = value }
        fun displayId(value: Int) = apply { this.displayId = value }
        fun build() = Parameters(projection, screenshot, socket, quality, displayInfo, frameRate, displayId)
    }
}
