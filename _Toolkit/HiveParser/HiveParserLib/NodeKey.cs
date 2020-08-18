// NodeKey.cs

using System;
using System.IO;
using System.Collections.Generic;

namespace HiveParserLib
{
    public sealed class NodeKey
    {
        public NodeKey(BinaryReader hive)
        {
            ReadNodeStructure(hive);
            ReadChildNodes(hive);
            ReadChildValues(hive);
            ReadSecurityKey(hive);
        }

        // ReadNodeStructure
        // Parse basic node structure data.
        private void ReadNodeStructure(BinaryReader hive)
        {
            Byte[] buffer = hive.ReadBytes(4);

            // validate the NK header
            if (buffer[0] != 0x6E ||
                buffer[1] != 0x6B)
            {
                throw new MalformedHiveException("NK Header");
            }

            Int64 startingOffset = hive.BaseStream.Position;

            this.IsRootKey = (buffer[2] == 0x2C);
            this.Timestamp = DateTime.FromFileTime(hive.ReadInt64());

            // skip metadata
            hive.BaseStream.Position += 4;

            this.ParentOffset = hive.ReadInt32();
            this.SubkeyCount = hive.ReadInt32();

            // skip metadata
            hive.BaseStream.Position += 4;

            this.LFRecordOffset = hive.ReadInt32();

            // skip metadata
            hive.BaseStream.Position += 4;

            this.ValueCount        = hive.ReadInt32();
            this.ValueListOffset   = hive.ReadInt32();
            this.SecurityKeyOffset = hive.ReadInt32();
            this.ClassnameOffset   = hive.ReadInt32();

            hive.BaseStream.Position = startingOffset + 68;

            this.NameLength = hive.ReadInt16();
            this.ClassnameLength = hive.ReadInt16();

            buffer = hive.ReadBytes(this.NameLength);
            this.Name = System.Text.Encoding.UTF8.GetString(buffer);

            hive.BaseStream.Position = this.ClassnameOffset + 4 + 4096;
            this.ClassnameData = hive.ReadBytes(this.ClassnameLength);
        }

        // ReadChildNodes
        // Parse child node data from current node.
        private void ReadChildNodes(BinaryReader hive)
        {
            // there are three distinct ways to point to child node keys:
            //  - RI -> root index
            //  - LF -> fast leaf
            //  - LH -> hash leaf

            this.ChildNodes = new List<NodeKey>();
            if (this.LFRecordOffset != -1)
            {
                hive.BaseStream.Position = 4096 + this.LFRecordOffset + 4;
                Byte[] buffer = hive.ReadBytes(2);

                // root index
                if (buffer[0] == 0x72 &&
                    buffer[1] == 0x69)
                {
                    Int16 count = hive.ReadInt16();
                    for (Int16 i = 0; i < count; ++i)
                    {
                        Int64 pos = hive.BaseStream.Position;
                        Int32 offset = hive.ReadInt32();

                        hive.BaseStream.Position = 4096 + offset + 4;
                        buffer = hive.ReadBytes(2);

                        if (!(buffer[0] == 0x6C &&
                            (buffer[1] == 0x66 || buffer[1] == 0x68)))
                        {
                            throw new MalformedHiveException("LF/LH Record at: " + hive.BaseStream.Position);
                        }

                        ParseChildNodes(hive);

                        // go to the next record list
                        hive.BaseStream.Position = pos + 4;
                    }
                }
                // fast leaf / hash leaf
                else if (buffer[0] == 0x6C &&
                    (buffer[1] == 0x66 || buffer[1] == 0x68))
                {
                    ParseChildNodes(hive);
                }
                else
                {
                    throw new MalformedHiveException("LF/LH/RI Record at: " + hive.BaseStream.Position);
                }
            }
        }

        private void ParseChildNodes(BinaryReader hive)
        {
            Int16 count = hive.ReadInt16();
            Int64 topOfList = hive.BaseStream.Position;

            for (Int16 i = 0; i < count; ++i)
            {
                hive.BaseStream.Position = topOfList + (8 * i);
                Int32 newOffset = hive.ReadInt32();

                // skip metadata
                hive.BaseStream.Position += 4;
                hive.BaseStream.Position = 4096 + newOffset + 4;

                NodeKey nk = new NodeKey(hive) { ParentNodeKey = this };
                this.ChildNodes.Add(nk);
            }

            hive.BaseStream.Position = topOfList + (8 * count);
        }

        // ReadChildValues
        // Parse child value data from current node.
        private void ReadChildValues(BinaryReader hive)
        {
            this.ChildValues = new List<ValueKey>();
            if (this.ValueListOffset != -1)
            {
                hive.BaseStream.Position = 4096 + this.ValueListOffset + 4;
                for (Int32 i = 0; i < this.ValueCount; ++i)
                {
                    hive.BaseStream.Position = 4096 + this.ValueListOffset + 4 + (4 * i);
                    Int32 offset = hive.ReadInt32();
                    hive.BaseStream.Position = 4096 + offset + 4;
                    this.ChildValues.Add(new ValueKey(hive));
                }
            }
        }

        private void ReadSecurityKey(BinaryReader hive)
        {
            // reset the stream position to beginning of the SK record
            hive.BaseStream.Position = Constant.BaseBlockSize + this.SecurityKeyOffset;
            this.SecurityKey = new SecurityKey(hive);
        }

        public DateTime Timestamp { get; set; }
        public NodeKey ParentNodeKey { get; set; }
        public SecurityKey SecurityKey { get; set; }
        public List<NodeKey> ChildNodes { get; set; }
        public List<ValueKey> ChildValues { get; set; }

        public Int32 ParentOffset { get; set; }
        public Int32 SubkeyCount { get; set; }
        public Int32 LFRecordOffset { get; set; }
        public Int32 ClassnameOffset { get; set; }
        public Int32 SecurityKeyOffset { get; set; }
        public Int32 ValueCount { get; set; }
        public Int32 ValueListOffset { get; set; }
        public Int16 NameLength { get; set; }
        public Boolean IsRootKey { get; set; }
        public Int16 ClassnameLength { get; set; }
        public String Name { get; set; }
        public Byte[] ClassnameData { get; set; }
    }
}
