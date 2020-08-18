using System;

namespace HiveParserLib
{
    public sealed class HiveParserLibException : Exception
    {
        public HiveParserLibException(String msg)
            : base(msg)
        {

        }
    }
    public sealed class MalformedHiveException : Exception
    {
        public MalformedHiveException()
            : base("The specified registry hive file is invalid or corrupt")
        {

        }

        public MalformedHiveException(String location)
            : base(String.Format("The specified registry hive file is corrupt at location {0}", location))
        {

        }
    }

    public sealed class HiveTraversalException : Exception
    {
        public HiveTraversalException(String msg)
            : base(msg)
        {

        }
    }
}
