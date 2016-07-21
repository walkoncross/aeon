#!/usr/bin/env python
import os
import logging
from ingest.utils import get_files, write_manifest, convert_audio


logging.basicConfig()
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)


def main(input_directory, output_directory, transcript_directory,
         manifest_file):
    """ Convert all .flac files from subdirectories of input_directory to
    16-bit signed .wav files sampled at 16 kHz and store them in
    output_directory. Extract the transcript from the nearby .trans.txt file
    and store it in transcript_directory, then write a manifest file referring
    to each .wav file and its paired transcript.
    """

    def librispeech_flac_filename(filestr):
        parts = filestr.split("-")

        return os.path.join(input_directory, parts[0], parts[1],
                            "{}.flac".format(filestr))

    if not os.path.isdir(input_directory):
        raise IOError("Data directory does not exist! {}".format(input_directory))

    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    if not os.path.exists(transcript_directory):
        os.makedirs(transcript_directory)

    transcript_files = get_files(input_directory, pattern="*.txt")
    if len(transcript_files) == 0:
        logger.error("No .txt files were found in {}".format(input_directory))
        return

    logger.info("Beginning audio conversions")
    wav_files = list()
    txt_files = list()
    for ii, tfile in enumerate(transcript_files):
        # transcript file specifies transcript and flac filename for all librispeech files
        logger.info("Converting audio corresponding to transcript "
                    "{} of {}".format(ii, len(transcript_files)))
        with open(tfile, "r") as fid:
            lines = fid.readlines()

        for line in lines:
            filestr, transcript = line.split(" ", 1)
            flac_file = librispeech_flac_filename(filestr)
            wav_file = os.path.join(output_directory, "{}.wav".format(filestr))
            txt_file = os.path.join(transcript_directory,
                                    "{}.txt".format(filestr))

            # Convert flac to 16k wav
            success = convert_audio(flac_file, wav_file)
            if success is False:
                logger.warn("Audio conversion failed for {}".format(fname))
                continue

            # Write out short transcript file
            with open(txt_file, "w") as fid:
                fid.write(transcript)

            # Add to output lists to be written to manifest
            wav_files.append(wav_file)
            txt_files.append(txt_file)

    logger.info("Writing manifest file to {}".format(manifest_file))
    return write_manifest(manifest_file, wav_files, txt_files)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest_file",
                        help="Output file that specifies the filename for each"
                        " input .wav and output .txt")
    parser.add_argument("input_directory",
                        help="Directory containing librispeech flac files")
    parser.add_argument("output_directory",
                        help="Directory to write ingested .wav files")
    parser.add_argument("transcript_directory",
                        help="Directory to write transcript .txt files")

    args = parser.parse_args()
    main(args.input_directory,
         args.output_directory,
         args.transcript_directory,
         args.manifest_file)
