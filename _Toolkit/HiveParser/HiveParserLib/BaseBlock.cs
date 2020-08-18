// BaseBlock.cs

using System;
using System.IO;

namespace HiveParserLib
{
    public sealed class BaseBlock
    {
        public BaseBlock(BinaryReader reader)
        {
            // reset the reader stream position
            reader.BaseStream.Position = 4;

            this.PrimarySequence   = reader.ReadInt32();                          // 8
            this.SecondarySequence = reader.ReadInt32();                          // 12
            this.LastWritten       = DateTime.FromFileTime(reader.ReadInt64());   // 20
            ;

            reader.BaseStream.Position += 16;             // 36

            this.RootCellOffset   = reader.ReadInt32();   // 40
            this.HiveBinsDataSize = reader.ReadInt32();   // 44
        }

        public Int32 PrimarySequence { get; set; }
        public Int32 SecondarySequence { get; set; }
        public DateTime LastWritten { get; set; }
        public Int32 RootCellOffset { get; set; }
        public Int32 HiveBinsDataSize { get; set; }
    }
}
