// ConsoleClient.cs
// Simple console client application for registry hive parsing library.

using System;
using System.Security.Principal;

using HiveParserLib;

namespace ConsoleClient
{
    class ConsoleClient
    {
        static void Main(string[] args)
        {
            RegistryHive hive = new RegistryHive(args[0]);
            Console.WriteLine("The root key's name is: " + hive.RootKey.Name);
            Console.WriteLine(hive.RootKey.SecurityKey.Descriptor.Dacl.AceCount);

            // GetSystemUsers(hive);
        }

        static void GetSystemUsers(RegistryHive samHive)
        {
            try
            {
                Utilities.GetSystemUsers(samHive);
            }
            catch (HiveParserLibException e)
            {
                Console.WriteLine("Failed to enumerate system users:");
                Console.WriteLine(e.Message);
            }
        }

        static void GetBootKey(RegistryHive systemHive)
        {
            Byte[] bootkey = null;

            try
            {
                bootkey = Utilities.GetBootKey(systemHive);
            }
            catch (HiveParserLibException e)
            {
                Console.WriteLine("Failed to extract boot key:");
                Console.WriteLine(e.Message);
            }

            if (bootkey != null)
            {
                Console.WriteLine("Boot key: " + BitConverter.ToString(bootkey));
            }
        }

        static void GetInstalledSoftware(RegistryHive softwareHive)
        {
            try
            {
                Utilities.GetInstalledSoftware(softwareHive);
            }
            catch (HiveParserLibException e)
            {
                Console.WriteLine("Failed to enumerate system software:");
                Console.WriteLine(e.Message);
            }
        }
    }
}
