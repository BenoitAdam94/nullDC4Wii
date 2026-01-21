using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace shader_gen
{
    using u32 = UInt32;
    using System.IO;
    class Program
    {
        static u32 GetShaderNum(u32 UseAlpha, u32 Texture, u32 IgnoreTexA, u32 ShadInstr, u32 Offset)
        {
            u32 rv = 0;
            rv |= UseAlpha; rv <<= 1;
            rv |= Texture; rv <<= 1;
            rv |= IgnoreTexA; rv <<= 2;
            rv |= ShadInstr; rv <<= 1;
            rv |= Offset;

            return rv;
        }

        static void Main(string[] args)
        {
            string Shader = File.ReadAllText("pixel.cg");

            for (u32 UseAlpha = 0; UseAlpha < 2; UseAlpha++)
            {
                for (u32 Texture = 0; Texture < 2; Texture++)
                {
                    bool fst = true;
                    for (u32 IgnoreTexA = 0; IgnoreTexA < 2; IgnoreTexA++)
                    {
                        for (u32 ShadInstr = 0; ShadInstr < 4; ShadInstr++)
                        {
                            for (u32 Offset = 0; Offset < 2; Offset++)
                            {
                                if (Texture == 0 && !fst)
                                    fst = fst;
                                else
                                {
                                    u32 sid= GetShaderNum(UseAlpha, Texture, IgnoreTexA, ShadInstr, Offset);
                                    string file="pixel_" + sid + ".cg";
                                    string data=
                                        "#define pp_UseAlpha " + UseAlpha + "\n" +
                                        "#define pp_Texture "+ Texture + "\n" +
                                        "#define pp_IgnoreTexA "+ IgnoreTexA + "\n" +
                                        "#define pp_ShadInstr "+ ShadInstr + "\n" +
                                        "#define pp_Offset " + Offset +  "\n";

                                    File.WriteAllText(file, data + Shader.Replace("ps_main_%d","ps_main_" + sid));
                                    Console.Write("shaders/" + file + " ");
                                }
                                fst = false;
                            }
                        }
                    }
                }
            }
        }
    }
}
