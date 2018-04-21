//
// Created by ezra on 12/6/17.
//

// TODO: looping/1shot behavior flag

#include <cmath>
#include <limits>


#include "interp.h"
#include "Resampler.h"

#include "SoftCutHeadLogic.h"


/// wtf...
#ifndef nullptr
#define nullptr ((void*)0)
#endif

using namespace softcuthead;

static int wrap(int val, int bound) {
    if(val >= bound) { return val - bound; }
    if(val < 0) { return val + bound; }
    return val;
}


SoftCutHeadLogic::SoftCutHeadLogic() {
    this->init();
}

void SoftCutHeadLogic::init() {
    sr = 44100.f;
    start = 0.f;
    end = 0.f;
    active = 0;
    phaseInc = 0.f;
    setFadeTime(0.1f);
    buf = (float*) nullptr;
    bufFrames = 0;
    // fadeMode = FADE_LIN;
    fadeMode = FADE_EQ;
    recRun = false;
}

void SoftCutHeadLogic::nextSample(float in, float *outPhase, float *outTrig, float *outAudio) {

    if(buf == nullptr) {
        return;
    }

    *outAudio = mixFade(head[0].peek(), head[1].peek(), head[0].fade(), head[1].fade());
    *outTrig = head[0].trig() + head[1].trig();
    if(outPhase != nullptr) { *outPhase = head[active].phase(); }

    if(recRun) {
        head[0].poke(in, pre, rec, fadePre, fadeRec);
        head[1].poke(in, pre, rec, fadePre, fadeRec);
    }

    SubHead::Action act0 = head[0].updatePhase(phaseInc, start, end, loopFlag);
    takeAction(act0, 0);
    SubHead::Action act1 = head[1].updatePhase(phaseInc, start, end, loopFlag);
    takeAction(act1, 1);

    head[0].updateFade(fadeInc);
    head[1].updateFade(fadeInc);
}


void SoftCutHeadLogic::setRate(float x)
{
    phaseInc = x;
}

void SoftCutHeadLogic::setLoopStartSeconds(float x)
{
    start = x * sr;
}

void SoftCutHeadLogic::setLoopEndSeconds(float x)
{
    end = x * sr;
}

/*
void SoftCutHeadLogic::updatePhase(int id)
{
    double p;
    trig[id] = 0.f;
    switch(state[id]) {
        case FADEIN:
        case FADEOUT:
        case ACTIVE:
            p = phase[id] + phaseInc;
            if(id == active) {
                if (phaseInc > 0.f) {
                    if (p > end || p < start) {
                        if (loopFlag) {
			  // FIXME: not sure whether or not to preserve phase overshoot
                            // cutToPos(start + (p-end));
                            cutToPhase(start);
                            trig[id] = 1.f;

                        } else {
                            state[id] = FADEOUT;
                        }
                    }
                } else { // negative rate
                    if (p > end || p < start) {
                        if(loopFlag) {
                            // cutToPos(end + (p - start));
                            cutToPhase(end);
                            trig[id] = 1.f;
                        } else {
                            state[id] = FADEOUT;
                        }
                    }
                } // rate sign check
            } // /active check
            phase[id] = p;
            break;
        case INACTIVE:
        default:
            ;; // nothing to do
    }
}
 */

void SoftCutHeadLogic::takeAction(SubHead::Action act, int id)
{
    switch (act) {
        case SubHead::LOOP_POS:
            cutToPhase(start);
            break;
        case SubHead::LOOP_NEG:
            cutToPhase(end);
            break;
        case SubHead::STOP:
        case SubHead::NONE:
        default: ;;
    }

}

void SoftCutHeadLogic::cutToPhase(float pos) {
    SubHead::State s = head[active].state();


    if(s == SubHead::State::FADEIN || s == SubHead::State::FADEOUT) { return; }
    int newActive = active == 0 ? 1 : 0;
    if(s != SubHead::State::INACTIVE) {
        head[active].setState(SubHead::State::FADEOUT);
    }
    head[newActive].setState(SubHead::State::FADEIN);
    //state[newActive] = FADEIN;
    //phase[newActive] = pos;
    head[newActive].setPhase(pos);
    active = newActive;
}

/*
void SoftCutHeadLogic::updateFade(int id) {
    switch(state[id]) {
        case FADEIN:
            fade[id] += fadeInc;
            if (fade[id] > 1.f) {
                fade[id] = 1.f;
                doneFadeIn(id);
            }
            break;
        case FADEOUT:
            fade[id] -= fadeInc;
            if (fade[id] < 0.f) {
                fade[id] = 0.f;
                doneFadeOut(id);
            }
            break;
        case ACTIVE:
        case INACTIVE:
        default:;; // nothing to do
    }
}
 */

void SoftCutHeadLogic::setFadeTime(float secs) {
    fadeInc = (float) 1.0 / (secs * sr);
}

/*
void SoftCutHeadLogic::doneFadeIn(int id) {
    state[id] = ACTIVE;
}

void SoftCutHeadLogic::doneFadeOut(int id) {
    state[id] = INACTIVE;
}

float SoftCutHeadLogic::peek(double phase) {
    return peek4(phase);
}


float SoftCutHeadLogic::peek4(double phase) {
    int phase1 = static_cast<int>(phase);
    int phase0 = phase1 - 1;
    int phase2 = phase1 + 1;
    int phase3 = phase1 + 2;

    double y0 = buf[wrap(phase0, bufFrames)];
    double y1 = buf[wrap(phase1, bufFrames)];
    double y2 = buf[wrap(phase2, bufFrames)];
    double y3 = buf[wrap(phase3, bufFrames)];

    double x = phase - (double)phase1;
    return static_cast<float>(cubicinterp(x, y0, y1, y2, y3));
}

void SoftCutHeadLogic::poke(float x, double phase, float fade) {
    double p = phase + recPhaseOffset;
    poke2(x, p, fade);
}
 */

// void SoftCutHeadLogic::poke0(float x, double phase, float fade) {
//     if (fade < std::numeric_limits<float>::epsilon()) { return; }
//     if (rec < std::numeric_limits<float>::epsilon()) { return; }

//     int phase0 = wrap((int) phase, bufFrames);

//     float fadeInv = 1.f - fade;

//     float preFade = pre * (1.f - fadePre) + fadePre * std::fmax(pre, (pre * fadeInv));
//     float recFade = rec * (1.f - fadeRec) + fadeRec * (rec * fade);

//     buf[phase0] *= preFade;
//     buf[phase0] += x * recFade;
// }

//void SoftCutHeadLogic::poke2(float x, double phase, float fade) {
//
//    // bail if record/fade level is ~=0, so we don't introduce noise
//    if (fade < std::numeric_limits<float>::epsilon()) { return; }
//    if (rec < std::numeric_limits<float>::epsilon()) { return; }
//
//    int phase0 = wrap(static_cast<int>(phase), bufFrames);
//    int phase1 = wrap(phase0 + 1, bufFrames);
//
//    float fadeInv = 1.f - fade;
//
//    float preFade = pre * (1.f - fadePre) + fadePre * std::fmax(pre, (pre * fadeInv));
//    float recFade = rec * (1.f - fadeRec) + fadeRec * (rec * fade);
//
//    float fr = static_cast<float>(phase - static_cast<int>(phase));
//
//    // linear-interpolated write values
//    //// FIXME: this could be better somehow
//    float x1 = fr*x;
//    float x0 = (1.f-fr)*x;
//
//    // mix old signal with interpolation
//    buf[phase0] = buf[phase0] * fr + (1.f-fr) * (preFade * buf[phase0]);
//    buf[phase1] = buf[phase1] * (1.f-fr) + fr * (preFade * buf[phase1]);
//
//    // add new signal with interpolation
//    buf[phase0] += x0 * recFade;
//    buf[phase1] += x1 * recFade;
//
//}

void SoftCutHeadLogic::setBuffer(float *b, uint32_t bf) {
    buf = b;
    bufFrames = bf;
}

void SoftCutHeadLogic::setLoopFlag(bool val) {
    loopFlag = val;
}

void SoftCutHeadLogic::setSampleRate(float sr_) {
    sr = sr_;
}

float SoftCutHeadLogic::mixFade(float x, float y, float a, float b) {
    if(fadeMode == FADE_EQ) {
        return x * sinf(a * (float) M_PI_2) + y * sinf(b * (float) M_PI_2);
    } else {
        return (x * a) + (y * b);
    }
}

void SoftCutHeadLogic::setRec(float x) {
    rec = x;
}


void SoftCutHeadLogic::setPre(float x) {
    pre= x;
}

void SoftCutHeadLogic::setFadePre(float x) {
    fadePre = x;

}

void SoftCutHeadLogic::setFadeRec(float x) {
    fadeRec = x;
}

void SoftCutHeadLogic::setRecRun(bool val) {
    recRun = val;
}

void SoftCutHeadLogic::setRecOffset(float x) {
    recPhaseOffset = x;
}
