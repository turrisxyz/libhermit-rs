(function() {var implementors = {};
implementors["num_complex"] = [{"text":"impl&lt;T:&nbsp;Clone + <a class=\"trait\" href=\"num_traits/trait.Num.html\" title=\"trait num_traits::Num\">Num</a> + Neg&lt;Output = T&gt;&gt; Neg for <a class=\"struct\" href=\"num_complex/struct.Complex.html\" title=\"struct num_complex::Complex\">Complex</a>&lt;T&gt;","synthetic":false,"types":["num_complex::Complex"]},{"text":"impl&lt;'a, T:&nbsp;Clone + <a class=\"trait\" href=\"num_traits/trait.Num.html\" title=\"trait num_traits::Num\">Num</a> + Neg&lt;Output = T&gt;&gt; Neg for &amp;'a <a class=\"struct\" href=\"num_complex/struct.Complex.html\" title=\"struct num_complex::Complex\">Complex</a>&lt;T&gt;","synthetic":false,"types":["num_complex::Complex"]}];
implementors["num_rational"] = [{"text":"impl&lt;T&gt; Neg for <a class=\"struct\" href=\"num_rational/struct.Ratio.html\" title=\"struct num_rational::Ratio\">Ratio</a>&lt;T&gt; <span class=\"where fmt-newline\">where<br>&nbsp;&nbsp;&nbsp;&nbsp;T: Clone + <a class=\"trait\" href=\"num_integer/trait.Integer.html\" title=\"trait num_integer::Integer\">Integer</a> + Neg&lt;Output = T&gt;,&nbsp;</span>","synthetic":false,"types":["num_rational::Ratio"]},{"text":"impl&lt;'a, T&gt; Neg for &amp;'a <a class=\"struct\" href=\"num_rational/struct.Ratio.html\" title=\"struct num_rational::Ratio\">Ratio</a>&lt;T&gt; <span class=\"where fmt-newline\">where<br>&nbsp;&nbsp;&nbsp;&nbsp;T: Clone + <a class=\"trait\" href=\"num_integer/trait.Integer.html\" title=\"trait num_integer::Integer\">Integer</a> + Neg&lt;Output = T&gt;,&nbsp;</span>","synthetic":false,"types":["num_rational::Ratio"]}];
implementors["time"] = [{"text":"impl Neg for <a class=\"struct\" href=\"time/struct.Duration.html\" title=\"struct time::Duration\">Duration</a>","synthetic":false,"types":["time::duration::Duration"]},{"text":"impl Neg for <a class=\"struct\" href=\"time/struct.UtcOffset.html\" title=\"struct time::UtcOffset\">UtcOffset</a>","synthetic":false,"types":["time::utc_offset::UtcOffset"]}];
if (window.register_implementors) {window.register_implementors(implementors);} else {window.pending_implementors = implementors;}})()