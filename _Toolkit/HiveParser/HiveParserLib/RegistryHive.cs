// RegistryHive.cs
// Queryable, in-memory representation of Windows registry hive.

using System;
using System.IO;

namespace HiveParserLib
{
    public sealed class RegistryHive
    {
        public RegistryHive(String path)
        {
            if (!File.Exists(path))
            {
                throw new FileNotFoundException();
            }

            this.Filepath = path;

            using (FileStream stream = File.OpenRead(path))
            {
                using (BinaryReader reader = new BinaryReader(stream))
                {
                    Byte[] buffer = reader.ReadBytes(4);

                    // perform basic validation of the hive
                    if (!ValidateHiveFile(buffer))
                    {
                        throw new MalformedHiveException();
                    }

                    // parse the hive base block
                    this.BaseBlock = new BaseBlock(reader);

                    // fast forward to end of registry header block
                    // skip the first four bytes of the hive bins section ('hbin')
                    reader.BaseStream.Position = 
                        Constant.BaseBlockSize + 
                        this.BaseBlock.RootCellOffset + 
                        4;

                    this.RootKey = new NodeKey(reader);
                }
            }
        }

        // ValidateHiveFile
        // Perform basic validation of hive file format.
        private Boolean ValidateHiveFile(Byte[] buffer)
        {
            return buffer[0] == 'r'
                && buffer[1] == 'e'
                && buffer[2] == 'g'
                && buffer[3] == 'f';
        }

        public String Filepath { get; set; }
        public NodeKey RootKey { get; set; }
        public BaseBlock BaseBlock { get; set; }
    }
}
