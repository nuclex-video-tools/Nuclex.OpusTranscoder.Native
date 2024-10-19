Iterative Declipper
===================

When you encode audio channels to Opus, even if those audio channels are free
of clipping, Opus may introduce some samples that go over the signal ceiling.
This is due to the internal workings of the codec and hard to predict.

The iterative declipper will encode the source audio track to Opus, then
decode the Opus file again and check for clipping. If clipping is discovered,
the source audio track is adjusted (not the decoded Opus one) by scaling down
the half-wave in which the clipping occurred.

Then another encode-decode round begins. The response of Opus to these
adjustments is non-linear. Since the correction factor of each half-wave is
tracked, if the signal still clips, the next round will calculate a stronger
correction factor.


Internals
---------

1. The input (original) audio file is decoded into memory

2. `ClippingDetector::FindClippingInstances()` identifies half-waves in which
   clipping samples exist in the original audio track.

   The volume quotient (value by which the volume gets divided) is initialized
   to 1.0 in this stage as the recorded peaks are present at a factor of 1.0.

(at this point, the untouched audio teack is encoded to Opus and then decoded
again, putting two separate copies of the audio track in memory: the untouched
original track and the track as it will looks when decoded from the Opus file)

for each iteration {

  3. `ClippingDetector::FindClippingInstances()` is called again to identify
     half-waves which are clipping in the Opus track.

  4. Clipping half-waves identified will not be directly added into the list,
     but their highest peak will be taken, then the half-wave around it will
     be staked out in the untouched track via `ClippingDetector::Integrate()`

     This is done because the zero crossings may not perfectly line up with
     the Opus output and we want our corrections to apply to whole half-waves
     and not cause sudden jumps in the middle of one.

  5. Now, for the first time, the actual values of the clipping half-waves
     are checked via `ClippingDetector::Update()`.

     This updates the `ClippingHalfwave::PeakAmplitude` for all half-waves
     recorded and ups the `ClippingHalfwave::IneffectiveIterationCount`
     counter that will ensure the algorithm will give up after a finite
     number of rectifying attempts.

  6. Here the exit condition for the loop is checked.
     `ClippingDetector::Update()` returns the number of remaining instancs
     of clipping in the track. If this is zero, the work is done.

  7. Finally, `HalfwaveTucker::TuckClippingHalfwaves` will do the de-clipping
     by going through all the recorded `ClippingHalfwave` instances (which
     at this stage are guaranteed to cover absolutely all instances of
     clipping and have peaks collected from the re-decoded Opus audio stream.

     If the measured peak is below 0 dB, the half-wave's volume quotient will
     be kept (it is fixed, after all).

     If the measured peak is still clipping, the volume quotient will be
     calculated based on the new information - using the volume quotient that
     currently being applied scaled by the observed effect.
