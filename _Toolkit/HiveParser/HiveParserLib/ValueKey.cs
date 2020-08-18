// ValueKey.cs

using System;
using System.IO;

namespace HiveParserLib
{
    public sealed class ValueKey
    {
        public ValueKey(BinaryReader hive)
        {
            Byte[] buffer = hive.ReadBytes(2);

            // vk
            if (buffer[0] != 0x76 ||
                buffer[1] != 0x6B)
            {
                throw new MalformedHiveException("VK Header");
            }

            this.NameLength = hive.ReadInt16();
            this.DataLength = hive.ReadInt32();

            Byte[] dataBuffer = hive.ReadBytes(4);

            this.ValueType = hive.ReadInt32();

            hive.BaseStream.Position += 4;

            buffer = hive.ReadBytes(this.NameLength);
            this.Name = (this.NameLength > 0) ?
                System.Text.Encoding.UTF8.GetString(buffer)
                : "Default";

            if (this.DataLength < 5)
            {
                this.Data = dataBuffer;
            }
            else
            {
                hive.BaseStream.Position = 4096 + BitConverter.ToInt32(dataBuffer, 0) + 4;
                this.Data = hive.ReadBytes(this.DataLength);
            }
        }

        public Int16 NameLength { get; set; }
        public Int32 DataLength { get; set; }
        public Int32 DataOffset { get; set; }
        public Int32 ValueType { get; set; }
        public String Name { get; set; }
        public Byte[] Data { get; set; }
    }
}
