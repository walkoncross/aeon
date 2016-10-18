/*
 Copyright 2016 Nervana Systems Inc.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include "etl_audio.hpp"

using namespace std;
using namespace nervana;

shared_ptr<audio::params> audio::param_factory::make_params(std::shared_ptr<const decoded>)
{
    auto audio_stgs = shared_ptr<audio::params>(new audio::params());

    audio_stgs->add_noise             = _cfg.add_noise(_dre);
    audio_stgs->noise_index           = _cfg.noise_index(_dre);
    audio_stgs->noise_level           = _cfg.noise_level(_dre);
    audio_stgs->noise_offset_fraction = _cfg.noise_offset_fraction(_dre);
    audio_stgs->time_scale_fraction   = _cfg.time_scale_fraction(_dre);

    return audio_stgs;
}

/** \brief Extract audio data from a wav file using sox */
std::shared_ptr<audio::decoded> audio::extractor::extract(const char* item, int itemSize)
{
    // std::cout << "Extracting " << item << std::endl;
    return make_shared<audio::decoded>(nervana::read_audio_from_mem(item, itemSize));
}

audio::transformer::transformer(const audio::config& config) :
    _cfg(config)
{
    if (_cfg.feature_type != "samples") {
        specgram::create_window(_cfg.window_type, _cfg.frame_length_tn, _window);
        specgram::create_filterbanks(_cfg.num_filters, _cfg.frame_length_tn, _cfg.sample_freq_hz,
                                     _filterbank);
    }
    _noisemaker = make_shared<noise_clips>(_cfg.noise_index_file);

}

audio::transformer::~transformer()
{
}


/** \brief Transform the raw sound waveform into the desired feature space,
* possibly after adding noise.
*
* The transformation pipeline is as follows:
* 1. Optionally add noise (controlled by add_noise parameter)
* 2. Convert to spectrogram
* 3. Optionally convert to MFSC (controlled by feature_type parameter)
* 4. Optionally convert to MFCC (controlled by feature_type parameter)
* 5. Optionally time-warp (controlled by time_scale_fraction)
*/
std::shared_ptr<audio::decoded> audio::transformer::transform(
                                      std::shared_ptr<audio::params> params,
                                      std::shared_ptr<audio::decoded> decoded)
{
    cv::Mat& samples_mat = decoded->get_time_data();
    _noisemaker->addNoise(samples_mat,
                          params->add_noise,
                          params->noise_index,
                          params->noise_offset_fraction,
                          params->noise_level); // no-op if no noise files

    if (_cfg.feature_type == "samples")
    {
        decoded->get_freq_data() = samples_mat;
        decoded->valid_frames = std::min((uint32_t) samples_mat.rows, (uint32_t) _cfg.time_steps);
    }
    else
    {
        // convert from time domain to frequency domain into the freq mat
        specgram::wav_to_specgram(samples_mat,
                                  _cfg.frame_length_tn,
                                  _cfg.frame_stride_tn,
                                  _cfg.time_steps,
                                  _window,
                                  decoded->get_freq_data());
        if (_cfg.feature_type != "specgram") {
            cv::Mat tmpmat;
            specgram::specgram_to_cepsgram(decoded->get_freq_data(), _filterbank, tmpmat);
            if (_cfg.feature_type == "mfcc") {
                specgram::cepsgram_to_mfcc(tmpmat, _cfg.num_cepstra, decoded->get_freq_data());
            } else {
                decoded->get_freq_data() = tmpmat;
            }
        }
        // std::cout << "First element is " << decoded->get_freq_data().at<float>(0, 0) << std::endl;
        // place into a destination with the appropriate time dimensions
        cv::Mat resized;
        cv::resize(decoded->get_freq_data(), resized, cv::Size(), 1.0, params->time_scale_fraction,
                   (params->time_scale_fraction > 1.0) ? CV_INTER_CUBIC : CV_INTER_AREA);
        decoded->get_freq_data() = resized;
        // std::cout << "First element is " << decoded->get_freq_data().at<float>(0, 0) << std::endl;

        // Add delta and delta-delta features
        // std::cout << "Getting delta features" << std::endl;
        if (_cfg.use_delta) {
            cv::Mat delta_mat;
            specgram::add_deltas(resized, 1,
                                 _cfg.use_delta_delta,
                                 delta_mat);
            // std::cout << "delta_mat has shape (" << delta_mat.rows << "," << delta_mat.cols << ")" << std::endl;
            decoded->get_freq_data() = delta_mat;
        }

        // std::cout << "First element is " << decoded->get_freq_data().at<float>(0, 0) << std::endl;
        // std::cout << "decoded has shape (" << decoded->get_freq_data().rows << "," << decoded->get_freq_data().cols << ")" << std::endl;
        decoded->valid_frames = std::min((uint32_t) decoded->get_freq_data().rows, (uint32_t) _cfg.time_steps);
        // std::cout << "valid frames is " << decoded->valid_frames << std::endl;
    }

    return decoded;
}

void audio::loader::load(const vector<void*>& outbuf, shared_ptr<audio::decoded> input)
{
    auto nframes = input->valid_frames;
    auto frames = input->get_freq_data();
    int cv_type = _cfg.get_shape_type().get_otype().cv_type;

    // std::cout << "Normalizing" << endl;
    if (_cfg.feature_type != "samples") {
        if (_cfg.use_delta) {
            int fbands = _cfg.use_delta_delta ? _cfg.freq_steps / 3 : _cfg.freq_steps / 2;
            // cout << "Normalizing first " << fbands << " bands from 0 to 255" << endl;
            // cv::normalize(frames(cv::Range(0, nframes),
            //                      cv::Range(0, fbands)),
            //               frames(cv::Range(0, nframes),
            //                      cv::Range(0, fbands)),
            //               0, 255, CV_MINMAX);
            // // cout << "Normalizing last bands from -255 to 255" << endl;
            // cv::normalize(frames(cv::Range(0, nframes),
            //                      cv::Range(fbands, _cfg.freq_steps)),
            //               frames(cv::Range(0, nframes),
            //                      cv::Range(fbands, _cfg.freq_steps)),
            //               -255, 255, CV_MINMAX);
        }
        else {
            // cv::normalize(frames(cv::Range(0, nframes),
            //                      cv::Range::all()),
            //               frames(cv::Range(0, nframes),
            //                      cv::Range::all()),
            //               0, 255, CV_MINMAX);
            cv::normalize(frames, frames, 0, 255, CV_MINMAX);
        }
    }

    // std::cout << "Creating padded" << endl;
    cv::Mat padded_frames(_cfg.time_steps, _cfg.freq_steps, cv_type);
    // std::cout << "padded has shape (" << _cfg.time_steps << "," << _cfg.freq_steps << ")" << endl;
    // std::cout << "Filling padded" << endl;
    frames(cv::Range(0, nframes), cv::Range::all()).convertTo(
        padded_frames(cv::Range(0, nframes), cv::Range::all()), cv_type);

    if (nframes < _cfg.time_steps) {
        // std::cout << "Padding" << endl;
        padded_frames(cv::Range(nframes, _cfg.time_steps), cv::Range::all()) = cv::Scalar::all(0);
    }
    // std::cout << "First element is " << padded_frames.at<float>(0, 0) << std::endl;
    // std::cout << "Moving to dst" << endl;
    cv::Mat dst(_cfg.freq_steps, _cfg.time_steps, cv_type, (void *) outbuf[0]);
    cv::transpose(padded_frames, dst);
    cv::flip(dst, dst, 0);
    // std::cout << "First element is " << dst.at<float>(0, 0) << std::endl;
}
