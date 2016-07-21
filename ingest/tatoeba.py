#!/usr/bin/env python
import os
import logging
from ingest.utils import write_manifest, get_files, convert_audio


logging.basicConfig()
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)


def get_all_transcripts(sentence_csv_file):
    """ Extracts all of the transcripts from the sentence details csv file
    """

    transcripts = dict()
    with open(sentence_csv_file, "r") as fid:
        for line in fid:
            details = line.split("\t")
            transcripts[details[0]] = details[2]

    return transcripts


def main(input_directory, sentence_details_file,
         output_directory, transcript_directory,
         manifest_file):
    """ Convert all .mp3 files from input_directory to 16-bit signed .wav files
    sampled at 16 kHz and store them in output_directory. Extract the transcript
    from sentence_details_file and store it in transcript_directory, then write
    a manifest file referring to each .wav file and its paired transcript.
    """

    # Set up all directories
    if not os.path.isdir(input_directory):
        raise IOError("Directory of mp3 files does not exist! {}".format(input_directory))

    if not os.path.isfile(sentence_details_file):
        raise IOError("Sentence details file does not exist! {}".format(sentence_details_file))

    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    if not os.path.exists(transcript_directory):
        os.makedirs(transcript_directory)

    # Get a dictionary of all of the transcripts for quick reference. This takes a bit of time but speeds things up considerably
    transcripts = get_all_transcripts(sentence_details_file)
    mp3_files = get_files(input_directory, "*.mp3", recursive=False)

    logger.info("Beginning audio conversions")
    wav_files = list()
    txt_files = list()
    for ii, mp3_file in enumerate(mp3_files):
        if (ii % 100) == 0:
            logger.info("Converting audio for file {}".format(ii))

        fname = os.path.splitext(os.path.basename(mp3_file))[0]

        # Get the transcript, if it exists
        try:
            tscript = transcripts[fname]
        except KeyError:
            logger.warn("Could not find transcript for {}".format(mp3_file))
            continue

        wav_file = os.path.join(output_directory, "{}.wav".format(fname))
        txt_file = os.path.join(transcript_directory, "{}.txt".format(fname))

        # Convert mp3 to 16k wav
        success = convert_audio(mp3_file, wav_file)
        if success is False:
            logger.warn("Audio conversion failed for {}".format(mp3_file))
            continue

        # Write out short transcript file
        with open(txt_file, "w") as fid:
            fid.write(tscript)

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
                        help="Directory containing the tatoeba mp3 files")
    parser.add_argument("sentence_details",
                        help="Path to sentence details .csv file")
    parser.add_argument("output_directory",
                        help="Directory to write ingested .wav files")
    parser.add_argument("transcript_directory",
                        help="Directory to write transcript .txt files")

    args = parser.parse_args()
    main(args.input_directory,
         args.sentence_details,
         args.output_directory,
         args.transcript_directory,
         args.manifest_file)
