package com.crzgames.testexe;

import org.libsdl.app.SDLActivity;

/**
 * A sample wrapper class that just calls SDLActivity
 */

public class MyGame extends SDLActivity {
    /**
     * Override this to load additional libraries
     */
    @Override
    protected String[] getLibraries() {
        return new String[] { 
            /**
             * Lib : SDL3 and related
             */
            "SDL3",
            "SDL3_image",
            "SDL3_mixer",
            "SDL3_ttf",
            /**
             * Lib : ffmpeg
             */
            "avcodec",
            "avdevice",
            "avfilter",
            "avformat",
            "avutil",
            "swresample",
            "swscale",
            /**
             * Lib : onnxruntime
             */
            "onnxruntime",
            "main"
        };
    }
}