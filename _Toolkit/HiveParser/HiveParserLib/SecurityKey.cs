// SecurityKey.cs

using System;
using System.IO;

namespace HiveParserLib
{
    public sealed class SecurityKey
    {
        public SecurityKey(BinaryReader reader)
        {
            this.Size = reader.ReadInt32();

            Byte[] buffer = reader.ReadBytes(2);
            if (buffer[0] != 0x73 ||
                buffer[1] != 0x6B)
            {
                throw new MalformedHiveException("SK Record at: " + reader.BaseStream.Position);
            }

            // skip padding, flink, blink
            reader.BaseStream.Position += 10;

            this.ReferenceCount   = reader.ReadUInt32();
            this.DescriptorLength = reader.ReadUInt32();

            if (this.DescriptorLength > 0)
            {
                this.Descriptor = new Descriptor(reader, this.DescriptorLength);
            }
        }

        public Int32 Size { get; set; }
        public UInt32 ReferenceCount { get; set; }
        public UInt32 DescriptorLength { get; set; }
        public Descriptor Descriptor { get; set; }
    }
}
