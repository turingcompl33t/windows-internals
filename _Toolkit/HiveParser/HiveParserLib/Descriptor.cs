// Descriptor.cs

using System;
using System.IO;
using System.Collections.Generic;
using System.Security.Principal;
using System.Security.AccessControl;

namespace HiveParserLib
{
    public sealed class Descriptor
    {
        public Descriptor(BinaryReader reader, UInt32 size)
        {
            Int64 descriptorBase = reader.BaseStream.Position;

            // skip padding
            reader.BaseStream.Position += 2;

            this.Control     = reader.ReadUInt16();
            this.OwnerOffset = reader.ReadUInt32();
            this.GroupOffset = reader.ReadUInt32();
            this.SaclOffset  = reader.ReadUInt32();
            this.DaclOffset  = reader.ReadUInt32();

            // compute raw data region sizes
            Int32 ownerSize = (Int32) (this.GroupOffset - this.OwnerOffset);
            Int32 groupSize = (Int32) (size - this.GroupOffset);

            // parse the owner SID
            reader.BaseStream.Position = descriptorBase + this.OwnerOffset;
            Byte[] buffer = reader.ReadBytes(ownerSize);
            this.OwnerSid = new SecurityIdentifier(buffer, 0);

            // parse the group SID
            reader.BaseStream.Position = descriptorBase + this.GroupOffset;
            buffer = reader.ReadBytes(groupSize);
            this.GroupSid = new SecurityIdentifier(buffer, 0);

            // determine if DACL and SACL are valid
            bool daclPresent = (this.Control & ((Int32)ControlFlags.DiscretionaryAclPresent)) > 0;
            bool saclPresent = (this.Control & ((Int32)ControlFlags.SystemAclPresent)) > 0;

            if (daclPresent)
            {
                this.Dacl = new AccessControlList(reader, descriptorBase, this.DaclOffset);
            }

            if (saclPresent)
            {
                this.Sacl = new AccessControlList(reader, descriptorBase, this.SaclOffset);
            }
        }

        public UInt16 Control { get; set; }
        public UInt32 OwnerOffset { get; set; }
        public UInt32 GroupOffset { get; set; }
        public UInt32 SaclOffset { get; set; }
        public UInt32 DaclOffset { get; set; }
        public SecurityIdentifier OwnerSid { get; set; }
        public SecurityIdentifier GroupSid { get; set; }
        public AccessControlList Dacl { get; set; }
        public AccessControlList Sacl { get; set; }
    }

    public sealed class AccessControlList
    {
        public AccessControlList(
            BinaryReader reader,
            Int64 descriptorBase,
            UInt32 offset
            )
        {
            // seek to beginning of ACL bytes
            reader.BaseStream.Position = descriptorBase + offset;
            reader.BaseStream.Position += 2;  // skip uninteresting bytes

            this.Size     = reader.ReadUInt16();
            this.AceCount = reader.ReadUInt16();

            for (UInt32 i = 0; i < this.AceCount; ++i)
            {
                // construct the new ACE
                AccessControlEntry temp = new AccessControlEntry(reader);

                // add the new ACE to the list
                AceList.Add(temp);

                // update the stream position based on the size of the processed entry
                reader.BaseStream.Position += temp.Size;
            }
        }

        public UInt16 Size { get; set; }
        public UInt16 AceCount { get; set; }
        public List<AccessControlEntry> AceList { get; set; }
    }

    public sealed class AccessControlEntry
    {
        public AccessControlEntry(BinaryReader reader)
        {
            this.Type  = reader.ReadByte();
            this.Flags = reader.ReadByte();
            this.Size  = reader.ReadUInt16();
            this.Mask  = reader.ReadUInt32();

            // remaining bytes of the ACE comprise the SID
            Byte[] buffer = reader.ReadBytes(this.Size - 8);

            this.Sid = new SecurityIdentifier(buffer, 0);
        }

        public Byte Type { get; set; }
        public Byte Flags { get; set; }
        public UInt16 Size { get; set; }
        public UInt32 Mask { get; set; }
        public SecurityIdentifier Sid { get; set; }
    }
}
