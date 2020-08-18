// Utilities.cs

using System;
using System.Linq;
using System.Text;

namespace HiveParserLib
{
    public sealed class Utilities
    {
        // GetBootKey
        // Extract the SYSTEM hive boot key.
        public static Byte[] GetBootKey(RegistryHive systemHive)
        {
            // determine the default control set used by the system
            ValueKey controlSet = null;
            
            try
            {
                controlSet = GetValueKey(systemHive, "Select\\Default");
            }
            catch (HiveTraversalException)
            {
                throw new HiveParserLibException(
                    "Failed to locate information required for boot key extraction; " +
                    "ensure the specified file is the SYSTEM hive and it is not corrupt");
            }

            Int32 cs = BitConverter.ToInt32(controlSet.Data, 0);

            // the loop below constructs the scrambled boot key
            StringBuilder scrambledKey = new StringBuilder();

            foreach (String key in new String[] { "JD", "Skew1", "GBG", "Data" })
            {
                NodeKey nk = GetNodeKey(systemHive, "ControlSet00" + cs + "\\Control\\Lsa\\" + key);

                for (Int32 i = 0; i < nk.ClassnameLength && i < 8; ++i)
                {
                    scrambledKey.Append((Char)nk.ClassnameData[i * 2]);
                }
            }

            Byte[] skey = StringToByteArray(scrambledKey.ToString());
            Byte[] descramble = new Byte[]
            {
                0x8, 0x5, 0x4, 0x2, 0xB, 0x9, 0xD, 0x3,
                0x0, 0x6, 0x1, 0xC, 0xE, 0xA, 0xF, 0x7
            };

            // construct the boot key
            Byte[] bootkey = new Byte[16];
            for (Int32 i = 0; i < bootkey.Length; ++i)
            {
                bootkey[i] = skey[descramble[i]];
            }

            return bootkey;
        }

        // GetInstalledSoftware
        // Query the information within the SOFTWARE hive and list
        // the installed software on the system.
        public static void GetInstalledSoftware(RegistryHive softwareHive)
        {
            NodeKey node;

            try
            {
                node = GetNodeKey(softwareHive, "Microsoft\\Windows\\CurrentVersion\\Uninstall");
            }
            catch (HiveTraversalException)
            {
                throw new HiveParserLibException("Failed to locate necessary key");
            }

            foreach (NodeKey child in node.ChildNodes)
            {
                Console.WriteLine("Found: " + child.Name);
                ValueKey val = child.ChildValues.SingleOrDefault(v => v.Name == "DisplayVersion");

                if (val != null)
                {
                    String version = System.Text.Encoding.UTF8.GetString(val.Data);
                    Console.WriteLine("\tVersion: " + version);
                }

                val = child.ChildValues.SingleOrDefault(v => v.Name == "InstallLocation");
                if (val != null)
                {
                    String location = System.Text.Encoding.UTF8.GetString(val.Data);
                    Console.WriteLine("\tLocation: " + location);
                }
            }
        }

        // GetSystemUsers
        // Query the information in the SAM hive and list system users.
        public static void GetSystemUsers(RegistryHive samHive)
        {
            NodeKey node;
            try
            {
                node = GetNodeKey(samHive, "SAM\\Domains\\Account\\Users\\Names");
            }
            catch (HiveTraversalException)
            {
                throw new HiveParserLibException("Failed to locate necessary key");
            }

            foreach (NodeKey child in node.ChildNodes)
            {
                Console.WriteLine(child.Name);
            }
        }

        // GetValueKey
        // Get ValueKey object by path.
        public static ValueKey GetValueKey(RegistryHive hive, String path)
        {
            String[] tokens = path.Split('\\');

            // separate the node path from the value key name
            String nodePath = String.Join("\\", tokens.Take(tokens.Length - 1));
            String keyname = tokens.Last();

            // get the node object for the desired node
            NodeKey node = GetNodeKey(hive, nodePath);

            return node.ChildValues.SingleOrDefault(v => v.Name == keyname);
        }

        // GetNodeKey
        // Get NodeKey object by path.
        public static NodeKey GetNodeKey(RegistryHive hive, String path)
        {
            NodeKey node = hive.RootKey;
            String[] paths = path.Split('\\');

            foreach (String ch in paths)
            {
                if (String.IsNullOrEmpty(ch))
                {
                    break;
                }

                // reset flag for this path element
                Boolean found = false;

                // iterate over the children of the current node,
                // searching for the node with name of current path element
                foreach (NodeKey child in node.ChildNodes)
                {
                    if (child.Name == ch)
                    {
                        // drop to next lower level in hierarchy
                        node = child;
                        found = true;
                        break;
                    }
                }

                if (found)
                {
                    continue;
                }
                else
                {
                    throw new HiveTraversalException("No Child Found With Name: " + ch);
                }
            }

            return node;
        }

        private static Byte[] StringToByteArray(String s)
        {
            return Enumerable.Range(0, s.Length)
                .Where(x => x % 2 == 0)
                .Select(x => Convert.ToByte(s.Substring(x, 2), 16))
                .ToArray();
        }
    }
}
