#include <SDL.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/avutil.h>
    #include <libavutil/opt.h>
    #include <libswscale/swscale.h>
    }

std::queue<AVFrame*> frameQueue;
std::mutex queueMutex;
std::condition_variable queueCv;
const size_t MAX_QUEUE_SIZE = 30;
bool running {true};

// Ideas for features:
// Seeking (back and forwards using arrow keys)
// Play/Pause
// Render frame number on each frame (and PTS, do we get those in a raw file?)
// Decoding of h264 mp4 files - next!
// SSIM of previous and current frames

void decode_frame()
{

}

void decoder_thread(const std::string & filename)
{
    // allocate a format context
    // this represents the container file and its streams
    AVFormatContext* fmtCtx = nullptr;
    int returnCode = avformat_open_input(&fmtCtx, /*filename.c_str()*/"/Users/sandea9/Downloads/bbb_sunflower_1080p_30fps_normal.mp4", nullptr, nullptr);
    if (returnCode < 0)
    {
        std::cout << "avformat_open_input failed!" << std::endl;
        return;
    }
    avformat_find_stream_info(fmtCtx, nullptr);

    // find the video stream
    int videoStreamIndex = -1;
    for (int i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    // find a decoder for the streams codec
    AVCodecParameters* codecPar = fmtCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);

    // allocate and open codec context
    // the codec context holds the state of the decoder
    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);
    avcodec_open2(codecCtx, codec, nullptr);

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    // read the frames, decode them and put them in the queue
    while (av_read_frame(fmtCtx, pkt) >= 0) {
        if (pkt->stream_index == videoStreamIndex) {
            avcodec_send_packet(codecCtx, pkt);    // Push packet to decoder
            
            while (avcodec_receive_frame(codecCtx, frame) == 0) {
                // frame->data and frame->linesize now contain raw decoded data
                // Typically YUV420P format for H.264
                // Push to your FrameQueue or render with SDL
                std::unique_lock<std::mutex> lock(queueMutex);
                // this unlocks the mutex if we don't meet the condition
                queueCv.wait(lock, [&] {return frameQueue.size() < MAX_QUEUE_SIZE;});
                // if we send frame, SDL could be rendering the frame whilst the decoder
                // thread overwrites it. We need instead to put copies of frame on the
                // queue and release the memory in the render loop
                frameQueue.push(av_frame_clone(frame));

                queueMutex.unlock();
                // this will notify the consumer thread that there's another frame on the
                // queue. This only does anything if the consumer thread is waiting on the
                // cv when the queue is too small
                queueCv.notify_all();
            }
        }
        av_packet_unref(pkt);
    }
}

// this can only run in the main thread of the program
void render_thread()
{
    try{
        // TODO: get width and height from video
        // create a window to render the frames onto
        SDL_Window * window = SDL_CreateWindow("YUV Player",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1920, 1080, SDL_WINDOW_SHOWN);

        if (window == nullptr)
        {
            std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        }

        // enable VSync. This means that the graphics system will wait until the display is
        // finished rendering the current frame before swapping in the new (back) buffer
        // This prevents tearing
        SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

        if (renderer == nullptr)
        {
            std::cerr << "Failed to create SDL renderer" << std::endl;
        }

        // create a texture to reuse whilst rendering
        // TODO: get the width and height here too
        SDL_Texture* texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_IYUV,  // YUV420P
            SDL_TEXTUREACCESS_STREAMING,
            1920, 1080
        );

        if (texture == nullptr)
        {
            std::cerr << "Failed to create SDL texture" << std::endl;
        }

        while (running)
        {
            // poll the SDL event loop
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    running = false;
                }
            }

            std::unique_lock<std::mutex> lock(queueMutex);
            // we're in the main thread so we shouldn't block
            queueCv.wait_for(lock, std::chrono::milliseconds(10), [&] {return !frameQueue.empty();});
            if (!frameQueue.empty())
            {
                // take a frame from the queue
                AVFrame * frame = frameQueue.front();
                frameQueue.pop();
                queueMutex.unlock();
                queueCv.notify_all();

                // turn it into an SDL texture
                SDL_UpdateYUVTexture(
                    texture, NULL,
                    frame->data[0], frame->linesize[0], // Y
                    frame->data[1], frame->linesize[1], // U
                    frame->data[2], frame->linesize[2]  // V
                );

                // render it
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, nullptr, nullptr);
                SDL_RenderPresent(renderer);

                // TODO: update this to use PTS
                // TODO: now we're in the main thread, we shouldn't block
                SDL_Delay(33);

                av_frame_free(&frame);
            }
            else
            {
                lock.unlock();
            }
        }

        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        SDL_Quit();
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in render thread: " << ex.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception in render thread" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return -1;
    }

    if (argc < 2)
    {
        std::cerr << "Not enough args, usage: main FILE WIDTH HEIGHT" << std::endl;
        SDL_Quit();
        return -1;
    }

    std::thread producer(decoder_thread, std::string(argv[1]));
    //std::thread consumer(render_thread);

    render_thread();

    producer.join();
    //consumer.join();

    return 0;
}