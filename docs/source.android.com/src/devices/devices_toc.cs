<!--
    Copyright 2014 The Android Open Source Project

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
-->
<?cs # Table of contents for devices.?>
<ul id="nav">

<!-- Porting Android -->
  <li class="nav-section">
    <div class="nav-section-header">
      <a href="<?cs var:toroot ?>devices/index.html">
        <span class="en">Interfaces</span>
      </a>
    </div>
    <ul>
      <li class="nav-section">
      <div class="nav-section-header">
        <a href="<?cs var:toroot ?>devices/audio.html">
          <span class="en">Audio</span>
        </a>
      </div>
        <ul>
          <li><a href="<?cs var:toroot ?>devices/audio_terminology.html">Terminology</a></li>
          <li><a href="<?cs var:toroot ?>devices/audio_implement.html">Implementation</a></li>
          <li><a href="<?cs var:toroot ?>devices/audio_attributes.html">Attributes</a></li>
          <li><a href="<?cs var:toroot ?>devices/audio_warmup.html">Warmup</a></li>
          <li class="nav-section">
            <div class="nav-section-header">
              <a href="<?cs var:toroot ?>devices/audio_latency.html">
                <span class="en">Latency</span>
              </a>
            </div>
            <ul>
              <li><a href="<?cs var:toroot ?>devices/audio_latency_measure.html">Measure</a></li>
              <li><a href="<?cs var:toroot ?>devices/latency_design.html">Design</a></li>
              <li><a href="<?cs var:toroot ?>devices/testing_circuit.html">Testing Circuit</a></li>
            </ul>
          </li>
          <li><a href="<?cs var:toroot ?>devices/audio_avoiding_pi.html">Priority Inversion</a></li>
          <li><a href="<?cs var:toroot ?>devices/audio_src.html">Sample Rate Conversion</a></li>
          <li><a href="<?cs var:toroot ?>devices/audio_debugging.html">Debugging</a></li>
          <li><a href="<?cs var:toroot ?>devices/audio_usb.html">USB Digital Audio</a></li>
          <li><a href="<?cs var:toroot ?>devices/audio_tv.html">TV Audio</a></li>
        </ul>
      </li>
      <li><a href="<?cs var:toroot ?>devices/bluetooth.html">Bluetooth</a></li>
      <li class="nav-section">
        <div class="nav-section-header">
          <a href="<?cs var:toroot ?>devices/camera/camera.html">
            <span class="en">Camera</span>
          </a>
        </div>
        <ul>
          <li><a href="<?cs var:toroot ?>devices/camera/camera3.html">Camera HAL3</a></li>
          <li><a href="<?cs var:toroot ?>devices/camera/camera3_requests_hal.html">HAL Subsystem</a></li>
          <li><a href="<?cs var:toroot ?>devices/camera/camera3_metadata.html">Metadata and Controls</a></li>
          <li><a href="<?cs var:toroot ?>devices/camera/camera3_3Amodes.html">3A Modes and State</a></li>
          <li><a href="<?cs var:toroot ?>devices/camera/camera3_crop_reprocess.html">Output and Cropping</a></li>
          <li><a href="<?cs var:toroot ?>devices/camera/camera3_error_stream.html">Errors and Streams</a></li>
          <li><a href="<?cs var:toroot ?>devices/camera/camera3_requests_methods.html">Request Creation</a></li>
          <li><a href="<?cs var:toroot ?>devices/camera/versioning.html">Version Support</a></li>
        </ul>
      </li>

      <li><a href="<?cs var:toroot ?>devices/drm.html">DRM</a></li>
      <li class="nav-section">
        <div class="nav-section-header">
          <a href="<?cs var:toroot ?>devices/tech/storage/index.html">
            <span class="en">External Storage</span>
          </a>
        </div>
        <ul>
          <li><a href="<?cs var:toroot ?>devices/tech/storage/config.html">Device Specific Configuration</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/storage/config-example.html">Typical Configuration Examples</a></li>
        </ul>
      </li>
      <li class="nav-section">
        <div class="nav-section-header">
          <a href="<?cs var:toroot ?>devices/graphics.html">
            <span class="en">Graphics</span>
          </a>
        </div>
        <ul>
          <li><a href="<?cs var:toroot ?>devices/graphics/architecture.html">Architecture</a></li>
          <li><a href="<?cs var:toroot ?>devices/graphics/implement.html">Implementation</a></li>
        </ul>
      </li>
      <li class="nav-section">
        <div class="nav-section-header">
          <a href="<?cs var:toroot ?>devices/tech/input/index.html">
            <span class="en">Input</span>
          </a>
        </div>
        <ul>
          <li><a href="<?cs var:toroot ?>devices/tech/input/overview.html">Overview</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/input/key-layout-files.html">Key Layout Files</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/input/key-character-map-files.html">Key Character Map Files</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/input/input-device-configuration-files.html">Input Device Configuration Files</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/input/migration-guide.html">Migration Guide</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/input/keyboard-devices.html">Keyboard Devices</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/input/touch-devices.html">Touch Devices</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/input/dumpsys.html">Dumpsys</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/input/getevent.html">Getevent</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/input/validate-keymaps.html">Validate Keymaps</a></li>
        </ul>
      </li>
      <li><a href="<?cs var:toroot ?>devices/media.html">Media</a></li>
     <li class="nav-section">
          <div class="nav-section-header">
            <a href="<?cs var:toroot ?>devices/sensors/index.html">
              <span class="en">Sensors</span>
            </a>
          </div>
          <ul>
            <li>
              <a href="<?cs var:toroot ?>devices/sensors/sensor-stack.html">
                <span class="en">Sensor stack</span>
              </a>
            </li>
            <li>
              <a href="<?cs var:toroot ?>devices/sensors/report-modes.html">
                <span class="en">Reporting modes</span>
              </a>
            </li>
            <li>
              <a href="<?cs var:toroot ?>devices/sensors/suspend-mode.html">
                <span class="en">Suspend mode</span>
              </a>
            </li>
            <li>
              <a href="<?cs var:toroot ?>devices/sensors/power-use.html">
                <span class="en">Power consumption</span>
              </a>
            </li>
            <li>
              <a href="<?cs var:toroot ?>devices/sensors/interaction.html">
                <span class="en">Interaction</span>
              </a>
            </li>
            <li>
              <a href="<?cs var:toroot ?>devices/sensors/hal-interface.html">
                <span class="en">HAL interface</span>
              </a>
            </li>
            <li>
              <a href="<?cs var:toroot ?>devices/sensors/batching.html">
                <span class="en">Batching</span>
              </a>
            </li>
            <li>
              <a href="<?cs var:toroot ?>devices/sensors/sensor-types.html">
                <span class="en">Sensor types</span>
              </a>
            </li>
            <li>
              <a href="<?cs var:toroot ?>devices/sensors/versioning.html">
                <span class="en">Version deprecation</span>
              </a>
            </li>
          </ul>
      </li>
      <li class="nav-section">
        <div class="nav-section-header">
          <a href="<?cs var:toroot ?>devices/tv/index.html">
            <span class="en">TV</span>
          </a>
        </div>
        <ul>
          <li><a href="<?cs var:toroot ?>devices/tv/HDMI-CEC.html">HDMI-CEC control service</a></li>
        </ul>
      </li>

    </ul>
  </li>
<!-- End Porting Android -->
  </li>


  <li class="nav-section">
    <div class="nav-section-header">
      <a href="<?cs var:toroot ?>devices/tech/index.html">
        <span class="en">Core Technologies</span>
      </a>
    </div>

    <ul>
      <li class="nav-section">
        <div class="nav-section-header">
          <a href="<?cs var:toroot ?>devices/tech/dalvik/index.html">
          <span class="en">ART and Dalvik</span></a>
        </div>
        <ul>
          <li><a href="<?cs var:toroot ?>devices/tech/dalvik/dalvik-bytecode.html">Bytecode Format</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/dalvik/dex-format.html">.Dex Format</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/dalvik/instruction-formats.html">Instruction Formats</a></li>
        </ul>
      </li>

      <li class="nav-section">
        <div class="nav-section-header">
          <a href="<?cs var:toroot ?>devices/tech/datausage/index.html">
            <span class="en">Data Usage</span>
          </a>
        </div>
        <ul>
          <li><a href="<?cs var:toroot ?>devices/tech/datausage/iface-overview.html">Network interface statistics overview</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/datausage/excluding-network-types.html">Excluding Network Types from Data Usage</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/datausage/tethering-data.html">Tethering Data</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/datausage/usage-cycle-resets-dates.html">Usage Cycle Reset Dates</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/datausage/kernel-overview.html">Kernel Overview</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/datausage/tags-explained.html">Data Usage Tags Explained</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/datausage/kernel-changes.html">Kernel Changes</a></li>
        </ul>
      </li>
      <li class="nav-section">
        <div class="nav-section-header">
          <a href="<?cs var:toroot ?>devices/debugtune.html">
            <span class="en">Debugging and Tuning</span>
          </a>
        </div>
        <ul>
          <li><a href="<?cs var:toroot ?>devices/tuning.html">Performance Tuning</a></li>
          <li><a href="<?cs var:toroot ?>devices/native-memory.html">Native Memory Usage</a></li>
        </ul>
      </li>

      <li class="nav-section">
        <div class="nav-section-header empty">
          <a href="<?cs var:toroot ?>devices/halref/index.html">
            <span class="en">HAL File Reference</span>
          </a>
        </div>
      </li>

      <li>
          <a href="<?cs var:toroot ?>devices/tech/kernel.html">
            <span class="en">Kernel</span>
          </a>
      </li>

      <li>
          <a href="<?cs var:toroot ?>devices/low-ram.html">
            <span class="en">Low RAM</span>
          </a>
      </li>

      <li>
          <a href="<?cs var:toroot ?>devices/tech/power.html">
            <span class="en">Power</span>
          </a>
      </li>

     <li class="nav-section">
          <div class="nav-section-header">
            <a href="<?cs var:toroot ?>devices/tech/security/index.html">
              <span class="en">Security</span>
            </a>
          </div>
        <ul>
            <li>
              <a href="<?cs var:toroot ?>devices/tech/security/acknowledgements.html">
                <span class="en">Acknowledgements</span>
              </a>
            </li>
          <li class="nav-section">
            <div class="nav-section-header">
              <a href="<?cs var:toroot ?>devices/tech/security/enhancements.html">
                <span class="en">Enhancements</span>
              </a>
            </div>
            <ul>
              <li><a href="<?cs var:toroot ?>devices/tech/security/enhancements50.html">Android 5.0</a></li>
              <li><a href="<?cs var:toroot ?>devices/tech/security/enhancements44.html">Android 4.4</a></li>
              <li><a href="<?cs var:toroot ?>devices/tech/security/enhancements43.html">Android 4.3</a></li>
              <li><a href="<?cs var:toroot ?>devices/tech/security/enhancements42.html">Android 4.2</a></li>
            </ul>
          </li>
            <li>
              <a href="<?cs var:toroot ?>devices/tech/security/best-practices.html">
                <span class="en">Best practices</span>
              </a>
            </li>
            <li>
              <a href="<?cs var:toroot ?>devices/tech/security/dm-verity.html">
                <span class="en">dm-verity on boot</span>
              </a>
            </li>
            <li>
              <a href="<?cs var:toroot ?>devices/tech/encryption/index.html">
                <span class="en">Encryption</span>
              </a>
            </li>
          <li class="nav-section">
            <div class="nav-section-header">
              <a href="<?cs var:toroot ?>devices/tech/security/se-linux.html">
                <span class="en">Security-Enhanced Linux</span>
              </a>
            </div>
            <ul>
              <li><a href="<?cs var:toroot ?>devices/tech/security/selinux/concepts.html">Concepts</a></li>
              <li><a href="<?cs var:toroot ?>devices/tech/security/selinux/implement.html">Implementation</a></li>
              <li><a href="<?cs var:toroot ?>devices/tech/security/selinux/customize.html">Customization</a></li>
              <li><a href="<?cs var:toroot ?>devices/tech/security/selinux/validate.html">Validation</a></li>
            </ul>
          </li>
          </ul>
      </li>

      <li class="nav-section">
        <div class="nav-section-header">
          <a href="<?cs var:toroot ?>devices/tech/test_infra/tradefed/index.html">
            <span class="en">Testing Infrastructure</span>
          </a>
        </div>
        <ul>
          <li><a href="<?cs var:toroot ?>devices/tech/test_infra/tradefed/fundamentals/index.html"
            >Start Here</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/test_infra/tradefed/fundamentals/machine_setup.html"
            >Machine Setup</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/test_infra/tradefed/fundamentals/devices.html"
            >Working with Devices</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/test_infra/tradefed/fundamentals/lifecycle.html"
            >Test Lifecycle</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/test_infra/tradefed/fundamentals/options.html"
            >Option Handling</a></li>
          <li><a href="<?cs var:toroot ?>devices/tech/test_infra/tradefed/full_example.html"
            >An End-to-End Example</a></li>
          <li id="tradefed-tree-list" class="nav-section">
            <div class="nav-section-header">
              <a href="<?cs var:toroot ?>reference/packages.html">
            <span class="en">Package Index</span>
          </a>
        <div>
      </li>
        </ul>
      </li>

    </ul>
  </li>

</ul>
