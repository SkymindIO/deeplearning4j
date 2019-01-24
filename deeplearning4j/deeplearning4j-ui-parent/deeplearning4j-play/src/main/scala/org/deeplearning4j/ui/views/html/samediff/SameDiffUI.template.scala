
package org.deeplearning4j.ui.views.html.samediff

import play.twirl.api._
import play.twirl.api.TemplateMagic._


     object SameDiffUI_Scope0 {
import models._
import controllers._
import play.api.i18n._
import views.html._
import play.api.templates.PlayMagic._
import play.api.mvc._
import play.api.data._

class SameDiffUI extends BaseScalaTemplate[play.twirl.api.HtmlFormat.Appendable,Format[play.twirl.api.HtmlFormat.Appendable]](play.twirl.api.HtmlFormat) with play.twirl.api.Template0[play.twirl.api.HtmlFormat.Appendable] {

  /**/
  def apply/*1.2*/():play.twirl.api.HtmlFormat.Appendable = {
    _display_ {
      {


Seq[Any](format.raw/*1.4*/("""
"""),format.raw/*2.1*/("""<!DOCTYPE html>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ~ Copyright (c) 2015-2019 Skymind, Inc.
  ~
  ~ This program and the accompanying materials are made available under the
  ~ terms of the Apache License, Version 2.0 which is available at
  ~ https://www.apache.org/licenses/LICENSE-2.0.
  ~
  ~ Unless required by applicable law or agreed to in writing, software
  ~ distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
  ~ WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
  ~ License for the specific language governing permissions and limitations
  ~ under the License.
  ~
  ~ SPDX-License-Identifier: Apache-2.0
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<html lang="en" style="height: 100%">
    <head>

        <meta charset="utf-8">
        <title>SameDiff Graph Visualization</title>
            <!-- start: Mobile Specific -->
        <meta name="viewport" content="width=device-width, initial-scale=1">
            <!-- end: Mobile Specific -->

        <link id="bootstrap-style" href="/assets/webjars/bootstrap/2.3.1/css/bootstrap.min.css" rel="stylesheet">

            <!-- The HTML5 shim, for IE6-8 support of HTML5 elements -->
            <!--[if lt IE 9]>
	  	<script src="http://html5shim.googlecode.com/svn/trunk/html5.js"></script>
		<link id="ie-style" href="/assets/css/ie.css" rel="stylesheet"/>
	<![endif]-->

            <!--[if IE 9]>
		<link id="ie9style" href="/assets/css/ie9.css" rel="stylesheet"/>
	<![endif]-->
    </head>

    <body style="height: 100%; margin: 0;">
            <!-- Start JavaScript-->
        <script src="/assets/webjars/jquery/2.2.0/jquery.min.js"></script>
        <script src="/assets/webjars/jquery-ui/1.10.2/ui/minified/jquery-ui.min.js"></script>
        <script src="/assets/webjars/bootstrap/2.3.1/js/bootstrap.min.js"></script>
        <script src="/assets/webjars/jquery-cookie/1.4.1-1/jquery.cookie.js"></script>
        <script src="/assets/webjars/dagre/0.8.4/dist/dagre.min.js"></script>
        <script src="/assets/webjars/cytoscape/3.3.3/dist/cytoscape.min.js"></script>
        <script src="/assets/webjars/cytoscape-dagre/2.1.0/cytoscape-dagre.js"></script>
        <script src="/assets/webjars/flatbuffers/1.9.0/js/flatbuffers.js"></script>

        <script src="/assets/js/samediff/generated/uigraphevents_generated.js"></script>
        <script src="/assets/js/samediff/generated/uigraphstatic_generated.js"></script>
        <script src="/assets/js/samediff/generated/array_generated.js"></script>
        <script src="/assets/js/samediff/generated/utils_generated.js"></script>
        <script src="/assets/js/samediff/generated/variable_generated.js"></script>

        <script src="/assets/js/samediff/samediff-ui.js"></script>
        <script src="/assets/js/samediff/flatbuffers-utils.js"></script>

        <div class="container-fluid-full">
            <div class="row-fluid">
                <input type="file" id="file" name="file" />
                <output id="list"></output>
                """),format.raw/*66.78*/("""
            """),format.raw/*67.13*/("""</div>
        </div>
        <div id="graphdiv" style="height:calc(100% - 60px); width: 100%; display: table">

        </div>


            <!-- Execute once on page load -->
        <script>
                document.getElementById('file').addEventListener('change', fileSelect, false);
                $(document).ready(function () """),format.raw/*77.47*/("""{"""),format.raw/*77.48*/("""
                    """),format.raw/*78.21*/("""renderSameDiffGraph();
                """),format.raw/*79.17*/("""}"""),format.raw/*79.18*/(""");
        </script>
    </body>
</html>
"""))
      }
    }
  }

  def render(): play.twirl.api.HtmlFormat.Appendable = apply()

  def f:(() => play.twirl.api.HtmlFormat.Appendable) = () => apply()

  def ref: this.type = this

}


}

/**/
object SameDiffUI extends SameDiffUI_Scope0.SameDiffUI
              /*
                  -- GENERATED --
                  DATE: Thu Jan 24 17:52:31 AEDT 2019
                  SOURCE: c:/DL4J/Git/deeplearning4j/deeplearning4j/deeplearning4j-ui-parent/deeplearning4j-play/src/main/views/org/deeplearning4j/ui/views/samediff/SameDiffUI.scala.html
                  HASH: e8b4f13064ac5abe94920c261d615a8913b48295
                  MATRIX: 561->1|657->3|685->5|3892->3245|3934->3259|4307->3604|4336->3605|4386->3627|4454->3667|4483->3668
                  LINES: 20->1|25->1|26->2|90->66|91->67|101->77|101->77|102->78|103->79|103->79
                  -- GENERATED --
              */
          