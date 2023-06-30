using System;
using System.Diagnostics;
using System.IO;
using static System.Net.Mime.MediaTypeNames;

class Program
{
    public static string? ProgramPath
    {
        get
        {
            string exePath = System.Reflection.Assembly.GetExecutingAssembly().Location;
            string exeDir = System.IO.Path.GetDirectoryName(exePath);
            return exeDir;
        }
    }
    static string XMLStart = "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"no\"?>\r\n<Stage xsi:noNamespaceSchemaLocation=\"http://web/chao/project/swa/schema/Stage.stg.xsd\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">";
    static string XMLEnd = "\r\n</Stage>";
    static public void Main(String[] args)
    {
        Console.Clear();
        WriteLineColors("Sonic Unleashed to Sonic Generations Stage Converter", ConsoleColor.Black, ConsoleColor.Cyan);
        WriteLineColors("Using ar0unpack and ar0pack from Skyth, HKXConverter from Team Unleashed, and XBCompress", ConsoleColor.DarkGray, ConsoleColor.Black);
        Console.WriteLine();
        WriteLineColors("INSTRUCTIONS", ConsoleColor.Black, ConsoleColor.White);
        WriteLineColors("1. Paste in the directories to the #Archive, the Archive and the Stage.pfd from the original stage\r\n2. When the program's done, unpack the normal archive, open GLVL 0.5.7 and do \"File > Fix all materials in folder\", select the unpacked archive and repack\r\n3. Load the stage, do Repack terrain geometry, untick Visibility Tree \r\n4. Delete visibility-tree.vt if it still exists\r\n5. Remove all stagepaths from the #archive or re-export them, these can make the geometry highly offset and invisible.\r\n6. If the geometry still won't load, check if all materials have valid shaders. Some skyboxes cause conflicts because of a missing \"DummySky\" shader", ConsoleColor.White, ConsoleColor.Black);
        Console.WriteLine();
        Console.WriteLine("Press any key to continue...");
        Console.ReadKey();
        Console.Clear();
        WriteLineColors("Enter path of the Unleashed level's #AR file:", ConsoleColor.Black, ConsoleColor.White);
        Console.ForegroundColor = ConsoleColor.DarkGray;
        var stringInput = Console.ReadLine();
        WriteLineColors("Enter path of the Unleashed level's AR file:", ConsoleColor.Black, ConsoleColor.White);
        Console.ForegroundColor = ConsoleColor.DarkGray;
        var stringInputNormal = Console.ReadLine();
        Console.ForegroundColor = ConsoleColor.Gray;
        if (Path.HasExtension(stringInput))
        {
            stringInput = ExtractAR(stringInput);
            Task.Delay(500);
        }
        if (Path.HasExtension(stringInputNormal))
        {
            stringInputNormal = ExtractAR(stringInputNormal);
            Task.Delay(500);
        }

        ConsoleProgressBar progress = new ConsoleProgressBar();        
        string newTerrainFile = "";
        string newInstancerFile = "";
        bool includeInTerrain = false;
        bool includeInInstancer = false;
        bool setData = false;
        string stagePath = Path.Combine(stringInput, "Stage.stg.xml").Replace("\"", "");
        string[] stageXMLContents = File.ReadAllLines(stagePath);
        List<string> newStageXML = stageXMLContents.ToList();

        foreach (var myString in stageXMLContents)
        {
            if (myString.Contains("<Terrain>"))
            {
                includeInTerrain = true;
            }
            else if (myString.Contains("</Sky>"))
            {
                includeInTerrain = false;
                newTerrainFile += $"\n{myString}";
                newStageXML.Remove(myString);
            }

            if (myString.Contains("<SetData>"))
            {
                setData = true;
            }
            else if (myString.Contains("</SetData>"))
            {
                setData = false;
                newStageXML.Remove(myString);
            }
            if (myString.Contains("<Instancer>"))
            {
                includeInInstancer = true;
            }
            else if (myString.Contains("</Instancer>"))
            {
                Console.WriteLine($"Deleted \"{myString}\"");
                includeInInstancer = false;
                newTerrainFile += $"\n{myString}";
                newStageXML.Remove(myString);
            }

            if (setData)
                newStageXML.Remove(myString);

            if (includeInTerrain)
            {
                Console.WriteLine($"Deleted \"{myString}\"");
                newTerrainFile += $"\n{myString}";
                newStageXML.Remove(myString);
            }
            if (includeInInstancer)
            {
                Console.WriteLine($"Deleted \"{myString}\"");
                newInstancerFile += $"\n{myString}";
                newStageXML.Remove(myString);

            }
        }

        newTerrainFile = newTerrainFile.Insert(0, XMLStart);
        newTerrainFile = newTerrainFile.Insert(newTerrainFile.Length, XMLEnd);

        newInstancerFile = newInstancerFile.Insert(0, XMLStart);
        newInstancerFile = newInstancerFile.Insert(newInstancerFile.Length, XMLEnd);

        File.Delete(Path.Combine(stringInput, "Terrain.prm.xml").Replace("\"", ""));
        File.WriteAllText(Path.Combine(stringInput, "Terrain.stg.xml").Replace("\"", ""), newTerrainFile);
        File.WriteAllText(Path.Combine(stringInput, "Instancer.stg.xml").Replace("\"", ""), newInstancerFile);
        File.WriteAllText(Path.Combine(stringInput, "Stage.stg.xml").Replace("\"", ""), string.Join("\n", newStageXML.ToArray()));

        //Move files and rename files to make it work
        var files = Directory.GetFiles(stringInput).Where(file => !file.EndsWith(".xml")).ToList();
        for (int i = 0; i < files.Count; i++)
        {
            var item = files[i];
            progress.SetProgress((i / (float)files.Count) * 100);
            try
            {
                File.Move(item, Path.Combine(stringInputNormal, Path.GetFileName(item)));
                Console.WriteLine($"Moved {Path.GetFileName(item)} from {new DirectoryInfo(stringInput).Name} to {new DirectoryInfo(stringInputNormal).Name}");

            }
            catch (Exception e)
            {
                Console.BackgroundColor = ConsoleColor.Red;
                Console.WriteLine(e.Message);
                Console.ResetColor();
            }
        }

        files = Directory.GetFiles(stringInput).Where(file => file.EndsWith(".xml")).ToList();
        for (int i = 0; i < files.Count; i++)
        {
            var item = files[i];

            progress.SetProgress((i / (float)files.Count) * 100);
            var file = Path.GetFileName(item);
            if (file.Contains("Base.set") || file.Contains("Sound.set") || file.Contains("Design.set") || file.Contains("Effect.set") || file.Contains("Break.set"))
            {
                string newName = Path.Combine(Path.GetDirectoryName(item), file.ToLower().Insert(0, "setdata_"));
                File.Move(item, newName);
                File.Delete(item);
                Console.WriteLine($"Converted {file} to {file.ToLower().Insert(0, "setdata_")}");
            }
        }
        WriteLineColors("Converting Havok files to 2010", ConsoleColor.Black, ConsoleColor.White);
        //for %%f in ("source/*.hkx") do hkxconverter source/%%f output/%%f
        //for %% f in ("output/*.hkx") do assetcc2--rules4101 output /%% f asset - cced /%% f
        files = Directory.GetFiles(stringInputNormal).Where(file => file.EndsWith(".hkx")).ToList();
        
        for (int i = 0; i < files.Count; i++)
        {
            progress.SetProgress((i / (float)files.Count) * 100);
            var item = files[i];
            Console.WriteLine($"Converting HKX... [{Path.GetFileName(item)}]");
            hkxconverter(item);
        }
        for (int i = 0; i < files.Count; i++)
        {
            progress.SetProgress((i / (float)files.Count) * 100);
            var item = files[i];
            Console.WriteLine($"Saving meta... [{Path.GetFileName(item)}]");
            assetcc2(item);
        }

        WriteLineColors("Enter a new name for the stage archives:", ConsoleColor.Black, ConsoleColor.White);
        var newNameFolder = Console.ReadLine();
        if(!string.IsNullOrEmpty(newNameFolder))
        {
            try
            {
                var e = Path.Combine(Directory.GetParent(stringInput).FullName, $"#{newNameFolder}");
                Directory.Move(stringInput, e);
                stringInput = e;
                e = Path.Combine(Directory.GetParent(stringInputNormal).FullName, newNameFolder);
                Directory.Move(stringInputNormal, e);
                stringInputNormal = e;
            }
            catch(Exception e)
            {
                Console.BackgroundColor = ConsoleColor.Red;
                Console.WriteLine(e.Message);
                Console.ResetColor();
            }
        }
        WriteLineColors("Enter path of the Unleashed level's Stage.pfd file: (optional, it's buggy)", ConsoleColor.Black, ConsoleColor.White);
        var stringInputPFD = Console.ReadLine();
        if (!string.IsNullOrEmpty(stringInputPFD))
        {
            stringInputPFD = ExtractAR(stringInputPFD);
            Task.Delay(5000);
            files = Directory.GetFiles(stringInputPFD).ToList();

            for (int i = 0; i < files.Count; i++)
            {
                progress.SetProgress((i / (float)files.Count) * 100);
                var item = files[i];
                string path = item;
                path = path.Insert(0, "\"");
                path = path.Insert(path.Length, "\"");
                xbdecompress(@path);
            }
            string pathFixed = stringInputPFD.Insert(0, "\"");
            pathFixed = pathFixed.Insert(pathFixed.Length, "\"");
            PackPFD(@pathFixed);
            if (File.Exists(Path.Combine(stringInputNormal, "Stage.pfi")))
                File.Delete(Path.Combine(stringInputNormal, "Stage.pfi"));
            Console.WriteLine($"Moving Stage.pfi into {new DirectoryInfo(stringInputNormal).Name}");
            File.Move(Path.Combine(Directory.GetParent(stringInputNormal).FullName, "Stage.pfi"), Path.Combine(stringInputNormal, "Stage.pfi"));
        }

        progress.Dispose();
        Console.WriteLine("Waiting 10 seconds to make sure everything's done...");
        Task.Delay(10000);
        PackAR(stringInput);
        Console.WriteLine($"Packing {new DirectoryInfo(stringInput).Name}");
        PackAR(stringInputNormal);
        Console.WriteLine($"Packing {new DirectoryInfo(stringInputNormal).Name}");

       
        

        
        Console.BackgroundColor = ConsoleColor.Green;
        Console.WriteLine("Conversion done!\r\nPress any key to exit...");
        Console.ReadKey();
        Console.BackgroundColor = ConsoleColor.Black;
        Console.ResetColor();
        Console.Clear();
    }

    static void hkxconverter(string path)
    {
        var tempPath = Directory.GetParent(path).FullName;
        tempPath += "_temp";
        tempPath = Path.Combine(tempPath, Path.GetFileName(path));
        ProcessStartInfo startInfo = new ProcessStartInfo();
        startInfo.FileName = @Path.Combine(ProgramPath, "hkxconverter.exe");
        startInfo.Arguments = $"\"{path}\" \"{tempPath}\"";
        startInfo.UseShellExecute = false;
        startInfo.RedirectStandardOutput = true;
        startInfo.CreateNoWindow = false;
        Process? extractPacked = Process.Start(startInfo);
        while (!extractPacked.StandardOutput.EndOfStream)
        {
            string line = extractPacked.StandardOutput.ReadLine();
            // do something with line
        }
        extractPacked.WaitForExit();
    }
    static void assetcc2(string path)
    {
        var tempPath = Directory.GetParent(path).FullName;
        tempPath += "_temp";
        tempPath = Path.Combine(tempPath, Path.GetFileName(path));
        //
        //assetcc2 --rules4101 output/%%f asset-cced/%%f
        ProcessStartInfo startInfo2 = new ProcessStartInfo();
        startInfo2.FileName = @Path.Combine(ProgramPath, "assetcc2.exe");
        startInfo2.Arguments = $"--rules4101 \"{tempPath}\" \"{path}\"";

        startInfo2.UseShellExecute = false;
        startInfo2.RedirectStandardOutput = true;
        startInfo2.CreateNoWindow = true;
        Process? extractPacked2 = Process.Start(startInfo2);
       
        extractPacked2.WaitForExit();
        
    }
    static void xbdecompress(string path)
    {
        ProcessStartInfo startInfo = new ProcessStartInfo();
        startInfo.FileName = @Path.Combine(ProgramPath, "xbdecompress.exe");
        startInfo.Arguments = $"  /S /F {path}";
        startInfo.UseShellExecute = false;
        startInfo.RedirectStandardOutput = true;
        startInfo.CreateNoWindow = true;
        Console.WriteLine($"Decompressing {Path.GetFileName(path)} from Stage.pfd...");
        Process? extractPacked = Process.Start(startInfo);
        extractPacked.WaitForExit();

        //File.Delete(@path);
        File.Move(Path.Combine(ProgramPath, Path.GetFileName(path)), path);
    }
    static string ExtractAR(string? path)
    {
        ProcessStartInfo startInfo = new ProcessStartInfo();
        startInfo.FileName = @Path.Combine(ProgramPath, "ar0unpack.exe");
        startInfo.Arguments = path;
        Process? extractPacked = Process.Start(startInfo);
        extractPacked.WaitForExit();

        path = Path.ChangeExtension(path, null);
        path = Path.ChangeExtension(path, null);
        path = path.Replace("\"", "");
        return path;
    }
    static void PackAR(string? path)
    {
        ProcessStartInfo startInfo = new ProcessStartInfo();
        startInfo.FileName = @Path.Combine(ProgramPath, "ar0pack.exe");
        startInfo.Arguments = $"\"{path}\"";
        Process? extractPacked = Process.Start(startInfo);
        extractPacked.WaitForExit();
    }
    static void PackPFD(string? path)
    {
        var output = path.Replace("Stage", "Stage_out");
        ProcessStartInfo startInfo = new ProcessStartInfo();
        startInfo.FileName = @Path.Combine(ProgramPath, "pfdpackUnleashed.exe");
        startInfo.Arguments = $"{output} {path}";
        Process? extractPacked = Process.Start(startInfo);
        
        extractPacked.WaitForExit();
    }
    public static void WriteLineColors(string line, ConsoleColor foreground, ConsoleColor background)
    {
        Console.BackgroundColor = background;
        Console.ForegroundColor = foreground;

        Console.WriteLine(line);
        Console.ResetColor();
    }
}