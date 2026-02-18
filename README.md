<!DOCTYPE html>
<html lang="en">
<tittle>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
</tittle>
<body>

<h1>Low Latency Application with C++</h1>

<p>This repository is dedicated to building a <strong>low-latency application</strong> in modern C++, heavily inspired by the book<br>
<strong>"Building Low Latency Applications with C++"</strong> by Sourav Ghosh.</p>

<p>The goal is to implement high-performance, ultra-low-latency components and techniques for time-critical systems (e.g., high-frequency trading, real-time processing, gaming engines, or any nanosecond-sensitive domain).</p>

<h2>Key Focus Areas</h2>
<ul>
  <li>Lock-free and wait-free data structures</li>
  <li>Efficient memory management and cache optimization</li>
  <li>Low-latency networking (kernel bypass, polling, etc.)</li>
  <li>Precise latency measurement and profiling</li>
  <li>Modern C++ best practices (C++20/23 features like concepts, coroutines, ranges)</li>
</ul>

<h2>Current Project Structure</h2>

<pre><code>low-latency/
├── src/                # Core source code (implementation of low-latency components)
├── CMakeLists.txt      # Build configuration using CMake
└── README.md           # This file (or README.html if using HTML version)
</code></pre>

<p>The <code>src/</code> folder contains the main C++ implementation files. More details and subfolders will be added as development progresses (e.g., benchmarks, tests, examples).</p>

<h2>Technologies & Tools</h2>
<ul>
  <li><strong>Language</strong>: C++20 / C++23</li>
  <li><strong>Build System</strong>: CMake</li>
  <li><strong>Compiler Recommendations</strong>: GCC 12+ or Clang 15+</li>
  <li>Planned additions:
    <ul>
      <li>Google Benchmark for micro-benchmarks</li>
      <li>Unit tests (Google Test or Catch2)</li>
      <li>Profiling: perf, flamegraph, Intel VTune</li>
    </ul>
  </li>
</ul>

<h2>How to Build (Basic Setup)</h2>

<ol>
  <li>Clone the repository:
    <pre><code>git clone https://github.com/VictorAugustoCorrea/low-latency.git
cd low-latency</code></pre>
  </li>
  <li>Create a build directory:
    <pre><code>mkdir build &amp;&amp; cd build</code></pre>
  </li>
  <li>Configure with CMake:
    <pre><code>cmake .. -DCMAKE_BUILD_TYPE=Release</code></pre>
  </li>
  <li>Build:
    <pre><code>cmake --build . --config Release -j$(nproc)</code></pre>
  </li>
  <li>(Future) Run executables or benchmarks from the <code>build/</code> folder.</li>
</ol>

<p><strong>Note</strong>: This is an early-stage project — build instructions will evolve as more components are added.</p>

<h2>Inspiration & Reference</h2>
<ul>
  <li><strong>Main Book</strong>: <em>Building Low Latency Applications with C++</em> – Sourav Ghosh<br>
    <a href="https://www.packtpub.com/product/building-low-latency-applications-with-c/9781837639359" target="_blank">Packt Publishing Link</a><br>
    (Covers building a complete low-latency trading system from scratch)
  </li>
</ul>

<h2>Status</h2>
<p><span class="emoji">🚧</span> <strong>Work in Progress</strong> – Early development phase<br>
Initial focus: Setting up CMake, basic low-latency primitives in <code>src/</code>, and experimenting with core concepts from the book.</p>

<h2>Contributing</h2>
<p>Feel free to open issues for suggestions, optimizations, or questions. Pull requests are welcome!</p>

<h2>License</h2>
<p>MIT License (see <a href=>LICENSE</a> file — add one if needed)</p>

<hr>

<p style="text-align: center; font-style: italic;">
  Focused on <strong>performance</strong>, <strong>learning</strong>, and applying real-world low-latency techniques.
</p>

</body>
</html>