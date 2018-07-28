# WebAssembly Experiments

This repository is a collection of personal projects used for me to learn WASM
(WebAssembly). I generally avoid web technologies like the plague, but I think
WASM offers a glimmer of hope.

I would like to see how far I can push WASM. Can WASM be used as a wholesale
replacement for HTML, CSS, and JavaScript? If not, what are the limitations of
WASM? Will WASM be able to emulate HTML/CSS features as efficiently as a web
browser? If WASM is not sufficient now, it feasible to improve WASM to come
closer to feature parity with core web technologies?

## Building and Running

Currently, building and running this code requires the Emscripten SDK.
Instructions for downloading, installing, and running the SDK can be found at
<https://kripken.github.io/emscripten-site/docs/getting_started/downloads.html>.

**TODO** finish this section

The Emscripten SDK has a lot more stuff than is necessary to compile code to
WASM. One of my goals is to learn the minimal number of tools and steps
required to compile code, so that the entire SDK is not required.

## Personal Thoughts

The rest of this README contains some of my personal thoughts and notes on
WASM. Though these are my personal opinions, I try to maintain a neutral tone,
and stick to technical facts when I can. If I state a fact incorrectly, please
feel free to correct my in a PR (Pull Request). However, please do not use PRs
to voice your own opinions. The purpose of PRs is to improve the quality of
this project, not to debate about which technology is better.

### The Web Today

I think most core web technologies, particularly frontend technologies, are
poorly designed. This includes HTML, CSS, JS (JavaScript), PHP, and the
innumerable frameworks written on top of them. This is not necessarily the
fault of the designers of these technologies, but more likely a consequence of
how the web evolved. Most of these technologies were developed by a single
person to accomplish a simple goal. Since these technologies are available on
the web, their use quickly became widespread. Over time, more people and
browsers adopted these technologies, and requested more features. However, the
web also requires that old features be maintained, so that old websites
continue to work. Thus, these technologies have become bloated in order to
satisfy backwards compatibility while offering new features at the same time.
The web has reached a point where it is no longer feasible for a single person,
or even a small team, to implement a web browser (from scratch). The technology
is too complex, and changes too quickly; even the major browsers have trouble
keeping up sometimes. This means that the entire web browsing experiencing is
dictated by only a few large parties, which some may argue is against some of
the core principles of the web, those of openness and freedom.

Web technologies are not only complicated to implement in a browser, but they
are problematic from a web developer and user standpoint as well. The HTML and
CSS standards are extremely complicated, and coaxing these technologies into
doing the right thing can be a challenge, even for experienced web developers.
Furthermore, HTML langauge in particular is awful. XML-like languages are
inefficient, because they contain a lot of redundant information, meaning they
require extra work for developers to type, and result in bloated file sizes.
Large files are particularly problematic on the web, because they have to be
transferred over a network connection. XML-like languages are also expensive to
parse. CSS is not XML based, so it is a little more efficient, but it is still
a text-based language, and text-based encodings are almost always less
efficient than a binary encoding.

JS has its own set of problems. In addition to being encoded as text, JS is an
interpreted, garbage collected language. JS VMs (Virtual Machines) and JITs
(Just In Time compilers) have come a long way to making it execute at a
reasonable speed, there is a theoretical and practical upper limit to how fast
an interpreted, garbage collected language can run. An experienced developer
working in a compiled language with manual memory management will be able to
produce faster code than a JS VM 99.99% of the time. Additionally, JS is a
dynamically typed language. Compared to a statically typed language, it is much
easier to make mistakes in a dynamically typed language, because the compiler
does less error checking.

Web frameworks (Angular, React, Node.js, Django, Ruby on Rails, etc.) attempt
to mask some of these problems, but they are all built on top of the same
flawed core web technologies, so they cannot fix some of their core problems.
These frameworks also tend to introduce new problems. For this reason, almost
every framework is hell to use, and has a short lifetime.

The user experience is harmed from the numerous problems of web technologies.
Websites take longer to download than they should, eat up more network
bandwidth than they should, use more memory than they should, take longer to
respond than they should, and use more CPU (and GPU) than they should. All of
things these have a negative impact on the users computer (Side note: this is
why frameworks like Electron, which embed an entire browser engine inside a
native application and use it for creating the user experience, are so
problematic). The web applications of today are always a worse experience than
a well-written native applications.

To make a long story short, the web is built on fundamentally flawed
technologies, and thus the user experience suffers. Some of these problems can
be reduced. For example, large files can be compressed and cached to reduce the
network bandwidth they require. Text editors can fill automatically fill in the
closing tags in HTML, making it easier to type. TypeScript can be used to
annotate JS with types to help catch errors at development time, instead of
leaving them for the user to find. However, these are tools and techniques are
akin to treating the symptom, not the disease.

Despite all of its problems, the web has some distinct advantages. First, the
web front end is (mostly) secure. Browsers occasionally have security bugs, but
are generally safer and more trusted than a random third party application.
Second, the web is convenient. Native applications must be downloaded, and
perhaps installed, which takes more time and steps from the user. Users must
also manually uninstall an application later, if they no longer want it.
Installed applications also tends to pollute the file system and other shared
system resource (like the Registry in Windows); these things don't always get
cleaned up when an application is uninstalled later. On the other hand, using a
web application is as simple as navigating to the right page; and once the user
leaves the page, the application is "uninstalled" (except perhaps for cached
data, though the user can manually clear this if they like). 

### Why WASM?

WASM carries the same benefits of existing web technologies - security and
convenience - while fixing a number of other problems. The user *interaction*
with a WASM-based webpage is identical to a JS-based one, but the user
*experience* is vastly improved. WASM-based webpages tends to use less memory,
use less CPU, have smaller file sizes, load faster, and be more responsive than
JS-based ones. It terms of efficiency, WASM is *almost* as good as native
languages.

Furthermore, WASM improves the developer experience. to write code in a safer
language. Officially supported languages include Rust, C, and C++; though any
language that targets LLVM, or WASM directly, could be used. In my opinion,
these languages are far superior to JS in developing robust, efficient
applications.

WASM is a far simpler technology than HTML, CSS, and JavaScript, yet it manages
to offer more freedom than all of these things combined. This gets back to some
of the core web principles.

### Downsides of WASM

Compared to native applications, WASM has some disadvantages:

* Native applications can directly access OS APIs, which offer more features
  and control. Some of these may features may eventually creep into WASM APIs,
  but others will not, due to security considerations and portability problems.
* WASM cannot do multithreading and SIMD, which is important for the
  performance of many applications. However, the WASM developers have promised
  to bring these features into WASM, so they will probably be arriving before
  not too long.
* WASM is more difficult to debug and profile, because it executes in a browser
  context. Browsers do not yet have sufficient tools and APIs to match the
  features of native code debuggers.
* WASM code is typically less efficient than native code. This is in part
  because it is a young technology, and WASM developers are still working out
  the best way to optimize WASM code. But this is also partly because of the
  WASM specification itself. The specification imposes security constraints
  (e.g. sandboxing) and portability requirements (e.g. it standardizes the
  behavior of floating point and integer operations) that causes WASM compilers
  to generate slow code in some circumstances.

WASM also has some disadvantages compared to JS, though these are because JS is
a more mature technology, and has gotten a lot of attention over the years.

* JS has more APIs available. JS code can be called from WASM code to access
  these APIs, but this has significant overhead, so it is not ideal. Browsers
  should be able to expose these APIs to directly to WASM without too much
  trouble. Over time, these APIs should be made available to WASM.
* JS has more mature development tools available, such as debuggers and
  profilers. Again, over time, the development ecosystem for WASM should
  improve.

### WASM Outside the Browser

WASM would be an ideal candidate for plugins to native applications. Existing
applications tend to use scripting languages, such as Python and Lua, to fill
this role; scripting languages are inefficient and usually have a heavyweight
runtime. Or these applications may allow native plugins, which carry a security
and instability risk unless they are carefully sandboxed. With a good
optimizer, WASM is nearly as efficient as native code, does not require a
heavyweight runtime, and is sandboxed by its design. Thus, it is unarguably
better than scripting languages for plugins; and offers an acceptable hit in
execution speed to offer security and robustness for the host application. This
allows users to trust WASM plugins downloaded from some random website on the
Internet with greater confidence than native plugins.

### Final Thoughts

In time, I believe WASM will mature to the point where JS is no longer
necessary (or even advantageous) to build a website. However, WASM will not be
able to replace native applications, in the same way C could not replace
assembly.

WASM may be able to replace other web technologies as well. For example, one
could build a 2D rendering engine using WebGL and WASM to replace HTML and CSS.
Such a rendering engine would not need to implement the entire HTML and CSS
specification, just the bits that are necessary to build a particular website.

If WASM can be used to obtain feature parity with existing web technologies,
and is at least as efficient as a browser at emulating these features, then the
future of the web could be based on WASM instead. If the web is based on WASM,
then browsers will be much simpler to implement. Browsers will also require
fewer new features, since WASM offers much more flexibility, meaning browsers
will be simpler to maintain and update. Developers will not need as many tools
to help them cope with the shortcomings of web technologies. And so on. All in
all, WASM is a better technology, and its use would help the rest of the web
ecosystem tremendously.
