using System;
using System.IO;
using System.Drawing;
using System.Drawing.Imaging;

class PpmToPng {
    static int ReadIntToken(BinaryReader br) {
        int b;
        do {
            b = br.ReadByte();
            if (b == '#') {
                while (br.ReadByte() != '\n') {}
            }
        } while (char.IsWhiteSpace((char)b));

        string s = ((char)b).ToString();
        while (true) {
            int p = br.PeekChar();
            if (p < 0 || char.IsWhiteSpace((char)p)) break;
            s += (char)br.ReadByte();
        }
        return int.Parse(s);
    }

    static void Convert(string ppmPath, string pngPath) {
        using (var fs = File.OpenRead(ppmPath))
        using (var br = new BinaryReader(fs)) {
            string magic = new string(br.ReadChars(2));
            if (magic != "P6") throw new Exception("Only binary PPM (P6) is supported");

            int width = ReadIntToken(br);
            int height = ReadIntToken(br);
            int maxval = ReadIntToken(br);
            if (maxval != 255) throw new Exception("Only maxval=255 supported");

            if (char.IsWhiteSpace((char)br.PeekChar())) br.ReadByte();

            using (var bmp = new Bitmap(width, height, PixelFormat.Format24bppRgb)) {
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        byte r = br.ReadByte();
                        byte g = br.ReadByte();
                        byte b = br.ReadByte();
                        bmp.SetPixel(x, y, Color.FromArgb(r, g, b));
                    }
                }

                bmp.Save(pngPath, ImageFormat.Png);
            }
        }
    }

    static int Main(string[] args) {
        if (args.Length != 2) {
            Console.Error.WriteLine("Usage: ppm_to_png <input.ppm> <output.png>");
            return 1;
        }
        try {
            Convert(args[0], args[1]);
            return 0;
        } catch (Exception ex) {
            Console.Error.WriteLine(ex.Message);
            return 1;
        }
    }
}
