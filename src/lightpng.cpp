#include <iostream>
#include <sys/time.h>
#include <zlib.h>
#include <png.h>
#include "lightpng.h"
#include "Image.h"
#include "PNGReader.h"
#include "JPEGReader.h"
#include "ReduceColor.h"
#include "PNGWriter.h"

void help()
{
    std::cout << "lightpng: PNG optimization tool for game graphics" << std::endl
              << "   Copyright (c) 2012 Yoshiki Shibukawa (DeNA Co.,Ltd, and ngmoco LLC)" << std::endl
              << "   Open source version" << std::endl
              << std::endl
              << "Texture file processing tool to use for texture." << std::endl
              << "  * Generetes small file size 16 bit PNG file." << std::endl
              << "  * It can generate emulated preview image in PNG format." << std::endl
              << std::endl
              << "usage> lightpng [options] input_image.png/jpg [output options]" << std::endl
              << std::endl
              << "  [options]" << std::endl
              << "   -s, --skip      : Skip optimization for PNG file size" << std::endl
              << "   -b, --benchmark : Display compression time" << std::endl
              << "   -v, --verbose   : Display compression result" << std::endl
              << "   -h, --help      : Show this message" << std::endl
              << std::endl
              << "  [output options]" << std::endl
              << "   -16a PATH     : 16 bit PNG with 4 bit alpha (RGBA 4444)" << std::endl
              << "    or -16 PATH  : If source image doesn't have alpha, it generates RGB 565 PNG." << std::endl
              << "   -16m PATH     : 16 bit PNG with 1 bit alpha (RGBA 5551)" << std::endl
              << "                 : If source image doesn't have alpha, it generates RGB 565 PNG." << std::endl
              << "   -32 PATH      : 24/32 bit PNG" << std::endl
              << "   -p16a PATH    : 16 bit PNG with 4 bit alpha (RGBA 4444) preview file" << std::endl
              << "    or -p16 PATH : 16 bit PNG with 4 bit alpha (RGBA 4444) preview file" << std::endl
              << "   -p16m PATH    : 16 bit PNG with 1 bit alpha (RGBA 5551) preview file" << std::endl
              << std::endl;
}

bool check_ext(std::string filename, const char* ext)
{
    std::string::size_type pos(filename.rfind('.'));
    if (pos != std::string::npos)
    {
        std::string fileext = filename.substr(pos, filename.length());
        return (fileext == ext);
    }
    return false;
}


double get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}


void parse_arg(int argc, const char** argv, const char*& input, output_list& outputs, bool& optimize, bool& bench, bool& verbose, Mode& mode, InputFileType& inputType)
{
    int state = 0;
    for (int i = 1; i < argc; ++i)
    {
        std::string opt(argv[i]);
        if (opt == "-h" or opt == "--help")
        {
            mode = helpMode;
            break;
        }
        if (state == 0)
        {
            if (opt == "-s" or opt == "--skip")
            {
                optimize = false;
            }
            else if (opt == "-b" or opt == "--benchmark")
            {
                bench = true;
            }
            else if (opt == "-v" or opt == "--verbose")
            {
                verbose = true;
            }
            else if (check_ext(opt, ".png"))
            {
                input = argv[i];
                inputType = PNGFile;
                state = 1;
            }
            else if (check_ext(opt, ".jpg") or check_ext(opt, ".jpeg"))
            {
                input = argv[i];
                inputType = JPEGFile;
                state = 1;
            }
            else
            {
                std::cout << "Unknown Parameter: " << opt << std::endl;
                mode = helpMode;
                break;
            }
        }
        else if (state == 1)
        {
            if ((opt == "-16a") or (opt == "-p16a") or (opt == "-16") or (opt == "-p16"))
            {
                i++;
                if (i == argc)
                {
                    std::cout << opt << " needs file path" << std::endl;
                    mode = helpMode;
                    break;
                }
                std::string path(argv[i]);
                if (check_ext(path, ".png"))
                {
                    if (opt == "-16a" or opt == "-16")
                    {
                        outputs.push_back(output_type(AlphaPNGFile, path));
                    }
                    else
                    {
                        outputs.push_back(output_type(PreviewAlphaPNGFile, path));
                    }
                }
                else
                {
                    std::cout << opt << " file should be .png " << path << std::endl;
                    mode = helpMode;
                    break;
                }
            }
            else if ((opt == "-16m") or (opt == "-p16m"))
            {
                i++;
                if (i == argc)
                {
                    std::cout << opt << " needs file path" << std::endl;
                    mode = helpMode;
                    break;
                }
                std::string path(argv[i]);
                if (check_ext(path, ".png"))
                {
                    if (opt == "-16m")
                    {
                        outputs.push_back(output_type(MaskPNGFile, path));
                    }
                    else
                    {
                        outputs.push_back(output_type(PreviewMaskPNGFile, path));
                    }
                }
                else
                {
                    std::cout << opt << " file should be .png " << path << std::endl;
                    mode = helpMode;
                    break;
                }
            }
            else if (opt == "-32")
            {
                i++;
                if (i == argc)
                {
                    std::cout << "-32 needs file path" << std::endl;
                    mode = helpMode;
                    break;
                }
                std::string path(argv[i]);
                if (check_ext(path, ".png"))
                {
                    outputs.push_back(output_type(FullColorPNGFile, path));
                }
                else
                {
                    std::cout << "-32 file should be .png " << path << std::endl;
                    mode = helpMode;
                    break;
                }
            }
            else
            {
                std::cout << "Unknown Parameter: " << opt << std::endl;
                mode = helpMode;
                break;
            }
        }
    }
}


void process_image(const char*& input_path, output_list& outputs, bool optimize, bool bench, bool verbose, Mode& mode, InputFileType& inputType)
{
    Image* reader;
    bool hasAlphaChannel = false;

    if (inputType == PNGFile)
    {
        reader = new PNGReader(input_path);
        hasAlphaChannel = (dynamic_cast<PNGReader*>(reader)->channels() == 4);
    }
    else
    {
        reader = new JPEGReader(input_path);
    }

    if (reader->valid())
    {
        PNGWriter *mask_png_writer = 0;
        PNGWriter *alpha_png_writer = 0;
        PNGWriter *noalpha_png_writer = 0;
        PNGWriter *preview_mask_png_writer = 0;
        PNGWriter *preview_alpha_png_writer = 0;
        PNGWriter *preview_noalpha_png_writer = 0;
        PNGWriter *fullcolor_png_writer = 0;
        for (output_list::iterator i = outputs.begin(); i != outputs.end(); ++i)
        {
            OutputFileType outputType = (*i).first;
            switch (outputType)
            {
            case MaskPNGFile:
                if (hasAlphaChannel)
                {
                    if (!mask_png_writer)
                    {
                        mask_png_writer = new PNGWriter(reader, hasAlphaChannel, optimize, verbose);
                        double t1 = get_time();
                        mask_png_writer->process(reduce_color<1>(reader, 5, 5, 5, false));
                        if (bench)
                        {
                            double t2 = get_time();
                            std::cout << "16bit PNG compression: " << t2 - t1 << std::endl;
                        }
                    }
                    mask_png_writer->write((*i).second.c_str());
                }
                else
                {
                    if (!noalpha_png_writer)
                    {
                        noalpha_png_writer = new PNGWriter(reader, hasAlphaChannel, optimize, verbose);
                        double t1 = get_time();
                        noalpha_png_writer->process(reduce_color<0>(reader, 5, 6, 5, false));
                        if (bench)
                        {
                            double t2 = get_time();
                            std::cout << "16bit PNG compression: " << t2 - t1 << std::endl;
                        }
                    }
                    noalpha_png_writer->write((*i).second.c_str());
                }
                break;
            case PreviewMaskPNGFile:
                if (hasAlphaChannel)
                {
                    if (!preview_mask_png_writer)
                    {
                        preview_mask_png_writer = new PNGWriter(reader, hasAlphaChannel, false, verbose);
                        double t1 = get_time();
                        preview_mask_png_writer->process(reduce_color<1>(reader, 5, 5, 5, true));
                        if (bench)
                        {
                            double t2 = get_time();
                            std::cout << "16bit PNG compression(Preview): " << t2 - t1 << std::endl;
                        }
                    }
                    preview_mask_png_writer->write((*i).second.c_str());
                }
                else
                {
                    if (!preview_noalpha_png_writer)
                    {
                        preview_noalpha_png_writer = new PNGWriter(reader, hasAlphaChannel, false, verbose);
                        double t1 = get_time();
                        preview_noalpha_png_writer->process(reduce_color<0>(reader, 5, 6, 5, true));
                        if (bench)
                        {
                            double t2 = get_time();
                            std::cout << "16bit PNG compression(Preview): " << t2 - t1 << std::endl;
                        }
                    }
                    preview_noalpha_png_writer->write((*i).second.c_str());
                }
                break;
            case AlphaPNGFile:
                if (hasAlphaChannel)
                {
                    if (!alpha_png_writer)
                    {
                        alpha_png_writer = new PNGWriter(reader, hasAlphaChannel, optimize, verbose);
                        double t1 = get_time();
                        alpha_png_writer->process(reduce_color<4>(reader, 4, 4, 4, false));
                        if (bench)
                        {
                            double t2 = get_time();
                            std::cout << "16bit PNG compression: " << t2 - t1 << std::endl;
                        }
                    }
                    alpha_png_writer->write((*i).second.c_str());
                }
                else
                {
                    if (!noalpha_png_writer)
                    {
                        noalpha_png_writer = new PNGWriter(reader, hasAlphaChannel, optimize, verbose);
                        double t1 = get_time();
                        noalpha_png_writer->process(reduce_color<0>(reader, 5, 6, 5, false));
                        if (bench)
                        {
                            double t2 = get_time();
                            std::cout << "16bit PNG compression: " << t2 - t1 << std::endl;
                        }
                    }
                    noalpha_png_writer->write((*i).second.c_str());
                }
                break;
            case PreviewAlphaPNGFile:
                if (hasAlphaChannel)
                {
                    if (!preview_alpha_png_writer)
                    {
                        preview_alpha_png_writer = new PNGWriter(reader, hasAlphaChannel, false, verbose);
                        double t1 = get_time();
                        preview_alpha_png_writer->process(reduce_color<4>(reader, 4, 4, 4, true));
                        if (bench)
                        {
                            double t2 = get_time();
                            std::cout << "16bit PNG compression(Preview): " << t2 - t1 << std::endl;
                        }
                    }
                    preview_alpha_png_writer->write((*i).second.c_str());
                }
                else
                {
                    if (!preview_noalpha_png_writer)
                    {
                        preview_noalpha_png_writer = new PNGWriter(reader, hasAlphaChannel, false, verbose);
                        double t1 = get_time();
                        preview_noalpha_png_writer->process(reduce_color<0>(reader, 5, 6, 5, true));
                        if (bench)
                        {
                            double t2 = get_time();
                            std::cout << "16bit PNG compression(Preview): " << t2 - t1 << std::endl;
                        }
                    }
                    preview_noalpha_png_writer->write((*i).second.c_str());
                }
                break;
            case FullColorPNGFile:
                if (!fullcolor_png_writer)
                {
                    fullcolor_png_writer = new PNGWriter(reader, hasAlphaChannel, true, verbose);
                    double t1 = get_time();
                    fullcolor_png_writer->process(reader->raw_image());
                    if (bench)
                    {
                        double t2 = get_time();
                        std::cout << "32bit PNG compression: " << t2 - t1 << std::endl;
                    }
                }
                fullcolor_png_writer->write((*i).second.c_str());
                break;
            }
        }
        delete reader;
        if (mask_png_writer)
        {
            delete mask_png_writer;
        }
        if (alpha_png_writer)
        {
            delete alpha_png_writer;
        }
        if (noalpha_png_writer)
        {
            delete noalpha_png_writer;
        }
        if (fullcolor_png_writer)
        {
            delete fullcolor_png_writer;
        }
        if (preview_mask_png_writer)
        {
            delete preview_mask_png_writer;
        }
        if (preview_alpha_png_writer)
        {
            delete preview_alpha_png_writer;
        }
        if (preview_noalpha_png_writer)
        {
            delete preview_noalpha_png_writer;
        }
    }
    else
    {
        std::cout << "Read error" << std::endl;
    }
}

int main(int argc, const char** argv)
{
    const char* input_path = 0;
    output_list outputs;
    Mode mode = textureMode;
    InputFileType inputType;
    bool optimize = true;
    bool bench = false;
    bool verbose = false;

    parse_arg(argc, argv, input_path, outputs, optimize, bench, verbose, mode, inputType);

    if (input_path == 0 || outputs.size() == 0)
    {
        mode = helpMode;
    }

    if (mode == helpMode)
    {
        help();
        return 1;
    }

    process_image(input_path, outputs, optimize, bench, verbose, mode, inputType);

    return 0;
}