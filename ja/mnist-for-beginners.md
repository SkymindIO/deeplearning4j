----
title:初心者のためのMNIST
layout: default
---

<header>
  <h1>初心者のためのMNIST</h1>
  <p>このチュートリアルでは、機械学習の世界ではまず最初に紹介されるべきMNISTデータセットの分類をします。</p>
  <ol class="toc">
    <li><a href="#introduction">はじめに</a></li>
    <li><a href="#mnist-dataset">MNISTのデータセット</a></li>
    <li><a href="#configuring">MNISTのサンプルの設定</a></li>
    <li><a href="#building">ニューラルネットワークの構築</a></li>
    <li><a href="#training">モデルのトレーニング</a></li>
    <li><a href="#evaluating">結果の評価</a></li>
    <li><a href="#conclusion">最後に</a></li>
  </ol>
</header>
  <p>このページを終了するのに必要な時間は約30分です。</p>
<section>
  <h2 id="introduction">はじめに</h2>
  <img src="/img/mnist_render.png"><br><br>
  <p>MNISTとは、手書き数字画像のデータベースです。各画像は整数によってラベル付けされています。機械学習アルゴリズムの性能のベンチマークとして使用されます。MNISTにおけるディープラーニングの性能は非常に良く、正答率は99.7%となっています。</p>
  <p>MNISTは、ニューラルネットワークが画像から数字を正しく推論できるようになるためのトレーニングに使用されます。まず最初にDeeplearning4jをインストールします。</p>
  <a href="quickstart" type="button" class="btn btn-lg btn-success" onClick="ga('send', 'event', equickstart', 'click');">DEEPLEARNING4Jを使用開始</a>
</section>

<section>
  <h2 id="mnist-dataset">MNISTのデータセット</h2>
  <p>MNISTのデータセットには、60,000のサンプルを含んだ<b>トレーニングセット</b>一部と10,000のサンプルを含んだ<b>テストセット</b>で構成されています。トレーニングセットは、アルゴリズムが正確なラベル、つまりこの場合は整数を推論できるようトレーニングするために使用され、テストセットは、トレーニングされたネットワークがどれだけの正答率で推論できるかをチェックするために使用されます。</p>
  <p>機械学習の世界では、これは<a href="https://ja.wikipedia.org/wiki/%E6%95%99%E5%B8%AB%E3%81%82%E3%82%8A%E5%AD%A6%E7%BF%92" target="_blank">教師あり学習（supervised learning）</a>と呼ばれています。推論する画像に正しい答えを提供することができるからです。したがって、この推論を間違えると、トレーニングセットが監督者または先生の役割を果たします。</p>
</section>

<section>
  <h2 id="configuring">MNISTの設定例</h2>
  <p>弊社にてMNISTのチュートリアルをMavenでパッケージ化したので、コードを記述する必要はありません。まずはIntelliJを開いてください。（IntelliJをダウンロードするには、弊社の<a href="quickstart">Quickstart…</a>をお読みください。）</p>
  <p>フォルダ<code>dl4j-examples</code>を開いてください。ディレクトリの<kbd>src</kbd> → <kbd>main</kbd> → <kbd>java</kbd> → <kbd>feedforward</kbd> → <kbd>mnist</kbd>へと進み、ファイルの<code>MLPMnistSingleLayerExample.java</code>を開いてください。</p>
  <p><img src="/img/mlp_mnist_single_layer_example_setup.png"></p>
  <p>このファイルでは、ニューラルネットワークを設定し、モデルのトレーニングを行い、結果を評価します。このコードをチュートリアルと一緒に確認すると役に立ちます。</p>
  <h3>変数の設定</h3>
  <pre><code class="language-java">
    final int numRows = 28; // 行列の行数
    final int numColumns = 28; // 行列の列数
    int outputNum = 10; // 可能な結果数（例：ラベルの0から9）
    int batchSize = 128; // 各ステップでいくつのサンプルを取り出すか
    int rngSeed = 123; // この乱数発生器はシードを適用することにより、同じ初期の重みがトレーニングに使用されていることを確保します。なぜこれが重要なのかについては後に説明します。
    int numEpochs = 15; // エポックとはデータセットが完全に通過した回数
  </code></pre>
  <p>弊社のサンプルでは、MNISTの各画像は28x28画素であり、 つまり入力データは28 <b>numRows</b> x 28 <b>numColumns</b>の行列（行列はディープラーニングのデータ構造の基盤）であるということになります。また、MNISTには可能な結果が10あります（0から9まで番号が付けられたラベル）。これは、弊社の<b>outputNum</b>に当たります。</p>
  <p><b>batchSize</b>と<b>numEpochs</b>は経験に基づいて選択します。これは実験を重ねていくにつれて分かってきます。高速トレーニングのバッチサイズが大きいとトレーニングが速くなり、エポックやデータセットの通過が多いと正答率が向上します。</p >
  <p>しかし、ある一定数を超えるエポックに対するリターンは減少するため、正答率とトレーニングの速度との間にはトレードオフがあります。一般には、最適な値を突き止めるまで実験を続ける必要があります。弊社はこのサンプルに妥当と思われるデフォルト値を設定しました。</p>
  <h3>MNISTのデータ</h3>
  <pre><code class="language-java">
    DataSetIterator mnistTrain = new MnistDataSetIterator(batchSize, true, rngSeed);
    DataSetIterator mnistTest = new MnistDataSetIterator(batchSize, false, rngSeed);
  </code></pre>
  <p><code>DataSetIterator</code>と呼ばれるクラスは、MNISTのデータセットを取り出すために使用されます。我々はある一つのデータセット<code>mnistTrain</code>を作成して<b>モデルのトーレーニング</b>を行い、もう一つのデータセット<code>mnistTest</code>を作成してトレーニング後のモデルの<b>正答率を評価</b>します。ところで、このモデルは、ニューラルネットワークのパラメータを参照します。これらのパラメータは、入力データの信号を処理する係数であり、ネットワークが各画像の正しいラベルを推論できるようになり、最終的に正確なモデルとなるまでこれらの係数は調節されます。</p>
</section>

<section>
  <h2 id="building">ニューラルネットワークの構築</h2>
  <p>あるフィードフォワード（順伝播型）ネットワークを<a href="http://jmlr.org/proceedings/papers/v9/glorot10a/glorot10a.pdf" target="_blank">Xavier Glorot and Yoshua Bengioによる論文</a>に基づいて構築しましょう。ここでは、隠れ層が一つだけの簡単な例で始めましょう。しかし、経験から言うと、ネットワークはディープであればあるほど（深ければ深いほど、つまり層が多ければ多いほど）、より複雑で微妙な部分を取り込み、正確な結果を出すことができます。</p>
  <img src="/img/onelayer.png"><br><br>
  <p>この図をよく覚えておいてください。というのは、これから我々はこのような一層で構成されたニューラルネットワークを構築するからです。</p>
  <h3>ハイパーパラメータの設定</h3>
  <p>Deeplearning4jで構築するいかなるニューラルネットワークも、基盤は<a href="http://deeplearning4j.org/neuralnet-configuration.html" target="_blank">NeuralNetConfigurationクラス</a>です。このクラスで、アーキテクチャの数量やアルゴリズムの学習方法を定義するハイパーパラメータを設定します。分かりやすい例で言うと、各パラメターはある料理に使う食材のうちの一つで、これによって料理が成功するか失敗するか、大きく左右されるようなものです。幸い、正しい結果が生み出されなければ、ハイパーパラメータを調整することができます。</p>
  <pre><code class="language-java">
    MultiLayerConfiguration conf = new NeuralNetConfiguration.Builder()
            .seed(rngSeed)
            .optimizationAlgo(OptimizationAlgorithm.STOCHASTIC_GRADIENT_DESCENT)
            .iterations(1)
            .learningRate(0.006)
            .updater(Updater.NESTEROVS).momentum(0.9)
            .regularization(true).l2(1e-4)
            .list()
  </code></pre>
  <h5>.seed(rngSeed)</h5>
  <p>このパラメータはある特定のランダムに生成された重み初期化を使用します。あるサンプルを何回も実行し、毎回開始時に新しい重みをランダムに生成すると、結果（F1値や正答率）にかなりの違いが生じるかもしれません。というのは、初期の重みが異なるとアルゴリズムでエラースケープの極小値が異なってしまうかもしれないからです。重みを同じランダムなものに保っておくと、他の条件を平等に保ったまま、他のハイパーパラメータをより正確に調節することによる効果のみに限って確認することができます。</p>
  <h5>.optimizationAlgo(OptimizationAlgorithm.STOCHASTIC_GRADIENT_DESCENT)</h4>
  <p>確率的勾配降下法（Stochastic gradient descent；SGD）は、コスト関数を最適化するのに一般に使用される方法です。エラーを最小限に抑える確率的勾配降下法や他の最適化アルゴリズムについて知るには、<a href="https://www.coursera.org/learn/machine-learning" target="_blank">Andrew Ng's Machine Learning course（Andrew Ng氏の機械学習コース）</a>、弊社の<a href="http://deeplearning4j.org/glossary#stochasticgradientdescent" target="_blank">グロッサリー</a>にある確率的勾配降下法の定義をご参照ください。</p>
  <h5>.iterations(1)</h5>
  <p>ニューラルネットワークのイテレーションを行うごとに、学習を一歩進めたことになります。つまり、モデルの重み更新を一回行ったことになるのです。ネットワークはデータを目の当たりにし、そのデータについて推論し、その推論がどのくらい間違っていたかに基づいて自身のパラメータを修正します。従って、イテレーションを多く行えば行うほど、ニューラルネットワークはより多く進歩し、学習し、エラーを最小限に抑えることができるのです。</p>
  <h5>.learningRate(0.006)</h5>
  <p>このコマンドで学習率を指定します。学習率とはイテレーション一回につき重みに対して行われた調節のサイズ、つまりステップサイズにあたります。学習率が高いとネットはerrorscapeを素早く巡回しますが、最小エラーのレベルを超えてしまう傾向があります。学習速度が低いと、最小値を見つける可能性は高まりますが、非常に遅い速度で行われます。重み調節が小さいステップで行われるためです。</p>
  <h5>.updater(Updater.NESTEROVS).momentum(0.9)</h5>
  <p>慣性項（momentum）は最適なポイントに最適化アルゴリズムがどれだけ素早く収束するかを決定する要素のうちの一つです。慣性項は、重みが調節される方向に影響するため、コーディングの世界では、一種の重みの<code>アップデーター</code>と見なします。</p>
  <h5>.regularization(true).l2(1e-4)</h5>
  <p>正規化（regularization）とは、<b>過剰適合（overfitting）</b>を回避するためのテクニックです。過剰適合とは、あるモデルがトレーニングのデータには非常によく適合しても、実際に使用された時に過去に接したことのないデータに出くわすやいなや非常に性能が悪くなることを言います。</p>
  <p>弊社では、L2正規化を使用することにより、個々の重みが全体の結果に大きな影響を及ぼさないよう回避しています。</p>
  <h5>.list()</h5>
  <p>ネットワーク内の層数を指定します。この関数は、設定をn回複製し、層に基づいた設定を構築します。</p>
  <p>上記の説明で分かりにくいことがあれば、先に挙げた<a href="https://www.coursera.org/learn/machine-learning" target="_blank">Andrew Ng's Machine Learning course（Andrew Ng氏の機械学習コース）</a>をご参照になることをお勧めします。</p>
  <h3>層の構築</h3>
  <p>ここでは各ハイパーパラメタ―の背景（活性化やweightInit）については取り上げませんが、それらの役割について簡単に触れておきましょう。ただし、これらがなぜ重要なのかを知りたい方は、<a href="http://jmlr.org/proceedings/papers/v9/glorot10a/glorot10a.pdf" target="_blank">Xavier Glorot and Yoshua Bengioによる論文</a>をお読みください。
  <img src="/img/onelayer_labeled.png"><br>
  <pre><code class="language-java">
    .layer(0, new DenseLayer.Builder()
            .nIn(numRows * numColumns) //入力データポイントの数
            .nOut(1000) // 出力データポイントの数
            .activation("relu") // 活性化関数
            .weightInit(WeightInit.XAVIER) // 重みの初期化
            .build())
    .layer(1, new OutputLayer.Builder(LossFunction.NEGATIVELOGLIKELIHOOD)
            .nIn(1000)
            .nOut(outputNum)
            .activation("softmax")
            .weightInit(WeightInit.XAVIER)
            .build())
    .pretrain(false).backprop(true)
    .build();
  </code></pre>
  <p>ところで隠れ層とは一体何なのでしょうか？</p>
  <p>隠れ層の各ノード（図式中の円）はMNISTデータセットでは、手書きの数字の特徴を表しています。例えば、数字の「6」を見ているとしましょう。その数字を表す丸い角を表すノード、交差した曲線を表すノードなどが一つづあるのです。これらの特徴はモデルの係数の重要性によって重みを付与され、この手書きの数字が実際に「6」なのか推論するために各隠れ層で再度組み合わせられます。ノードの層が多ければ多いほど、より正確な推論に必要な複雑さやニュアンスを取り込むことができます。</p>
  <p>「隠れ」層と言われるわけは、入力データがネット内に入り、決断が出てくるのを見ることができても、どのようにして、またなぜニューラルネットワークがデータを処理しているかが人間には解読不可能だからです。ニューラルネットワークのパラメーターは、機械のみが読み取ることのできる長い数字のベクトルに過ぎないのです。</p>
</section>

<section>
  <h2 id="training">モデルのトレーニング</h2>
  <p>モデルが構築できたら、トレーニング を開始しましょう。IntelliJの右上で、緑色の矢印をクリックします。この操作により、先に挙げたコードが実行されます。</p>
  <img src="/img/mlp_mnist_single_layer_example_training.png"><br><br>
  <p>これはハードウェアによっては、完了までに数分掛かることがあります。</p>
</section>

<section>
  <h2 id="evaluating">結果を評価</h2>
  <img src="/img/mlp_mnist_single_layer_example_results.png"><br><br>
  <p>
  <b>Accuracy（正答率）</b> - モデルが正しく識別したMNIST画像の割合<br>
  <b>Precision（適合率）</b> - 真陽性の数を真陽性と偽陽性の数で割った値<br>
  <b>Recall（再現率）</b> - 真陽性の数を真陽性の数と偽陰性の数で割った値<br>
  <b>F1値</b> - <b>適合率</b>と<b>再現率</b>の加重平均<br>
  </p>
  <p><b>正答率</b>はモデル全体を測定します。</p>
  <p><b>適合率、再現率、F1</b>はモデルの<b>適合性</b>を測定します。例えば、ある人がさらなる治療を求めないため、癌は再発しないだろうと（偽陰性）と予測するのは危険なことでしょう。このため、全体的に<b>正答率</b>が低めでも偽陰性（つまり適合率、再現率、F1が高め）を回避するモデルを選択するのが賢明でしょう。</p>
</section>

<section>
  <h2 id="conclusion">最後に</h2>
  <p>これで完了です！コンピュータビジョンが0ドメイン知識であるニューラルネットワークをトレーニングし、正答率の97.1%を達成しました。最先端の性能だと、これよりさらに優れており、ハイパーパラメターをさらに調節してモデルを改善させることができます。</p>
  <p>Deeplearning4jは、その他のフレームワークと比較すると、以下の点で優れています。</p>
  <ul>
    <li>規模を広げて、Spark、Hadoop、Kafkaなどの主要なJVMフレームワークと統合させることができる。</li>
    <li>分散CPU及び/または分散GPUでの実行に最適化されている。</li>
    <li>JavaやScalaのコミュニティに貢献している。</li>
    <li>導入された企業様への商業用サポートを行っている。</li>
  </ul>
  <p>ご質問のある方は弊社のオンライン<a href="https://gitter.im/deeplearning4j/deeplearning4j" target="_blank">Gitter サポートチャットルーム</a>にてご連絡ください。</p>
  <ul class="categorized-view view-col-3">
    <li>
      <h5>その他のDeeplearning4jのチュートリアル</h5>
      <a href="https://deeplearning4j.org/ja/neuralnet-overview">ディープニューラルネットワークについて</a>
      <a href="https://deeplearning4j.org/ja/restrictedboltzmannmachine">制限付きボルツマンマシンの初心者向けガイド</a>
      <a href="https://deeplearning4j.org/ja/eigenvector">固有ベクトル、主成分分析、共分散、エントロピー入門</a>
      <a href="https://deeplearning4j.org/ja/lstm">再帰型ネットワークと長・短期記憶についての初心者ガイド</a>
      <a href="https://deeplearning4j.org/ja/linear-regression">回帰を使ったニューラルネットワーク</a>
      <a href="https://deeplearning4j.org/ja/convolutionalnets">畳み込みネットワーク</a>
    </li>

    <li>
      <h5>おすすめのリソース</h5>
      <a href="https://www.coursera.org/learn/machine-learning/home/week/1">Andrew Ng's Online Machine Learning Course</a>
      <a href="https://github.com/deeplearning4j/dl4j-examples/blob/master/dl4j-examples/src/main/java/org/deeplearning4j/examples/convolution/LenetMnistExample.java">LeNet Example:MNIST With Convolutional Nets</a>
    </li>

  </ul>
</section>
