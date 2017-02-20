---
title: Deeplearning4j的自然语言处理功能
layout: cn-default
---

# Deeplearning4j的自然语言处理功能

由于设计目的不同，Deeplearning4j在自然语言处理方面可能无法与斯坦福的CoreNLP或NLTK相提并论，但它也包含一些重要的文本处理工具，我们将在本页中加以介绍。

Deeplearning4j的自然语言处理（NLP）功能依赖于[ClearTK](https://cleartk.github.io/cleartk/)——一个面向Apache[非结构化信息管理架构](https://uima.apache.org/)（UIMA）的机器学习与自然语言处理框架。UIMA能实现语言识别、特定语言的切分、语句边界检测和实体检测（检测专有名词：人名、企业名、地名和事物名）。 

### 语句迭代器

自然语言的处理包括几个步骤。首先要对语料库进行迭代，创建文档列表，而文档可以和微博一样短，也可以像报刊文章那么长。这一步由语句迭代器，即SentenceIterator来完成，代码如下： 

<script src="http://gist-it.appspot.com/https://github.com/deeplearning4j/dl4j-0.0.3.3-examples/blob/master/src/main/java/org/deeplearning4j/word2vec/Word2VecRawTextExample.java?slice=33:41"></script>

SentenceIterator封装一个语料库或一篇文本并对其进行整理，比如整理成每行一条微博的格式。它负责将文本一段一段地输入您的自然语言处理系统。虽然SentenceIterator的名称与DatasetIterator相似，但此二者不能加以类比。DatasetIterator用于创建神经网络的定型数据集，而SentenceIterator则通过切分语料库来创建一组字符串。 

### 分词器

分词器（Tokenizer）将文本进一步切分为单词，也可以切分为n-gram。ClearTK包括词性标注（PoS）和解析树（parse tree）等基础分词器，可以实现依赖解析和语法成分解析，与递归神经张量网络（RNTN）所使用的方法类似。 

分词器由[TokenizerFactory](https://github.com/deeplearning4j/deeplearning4j/blob/6f027fd5075e3e76a38123ae5e28c00c17db4361/deeplearning4j-scaleout/deeplearning4j-nlp/src/main/java/org/deeplearning4j/text/tokenization/tokenizerfactory/UimaTokenizerFactory.java)创建和包装。默认的词例（token）切分单位是以空格分隔的单词。分词过程也需要借助某些形式的机器学习来分辨具有多种意义的符号，比如“.”可能是句号，也有可能像在“Mr.”和“vs.”这样的词中那样表示缩写。

分词器和语句迭代器都需要和预处理器配合使用，以应对Unicode等较为杂乱的文本中出现的异常情况，将这类文本转换为统一的格式，比如全部转为小写字母。 

<script src="http://gist-it.appspot.com/https://github.com/deeplearning4j/dl4j-0.0.3.3-examples/blob/master/src/main/java/org/deeplearning4j/word2vec/Word2VecRawTextExample.java?slice=43:57"></script>



### 词汇表

每份文档都必须通过分词生成一个词汇表，其中包括该文档或语料库中比较重要的词。这些词是文档中所有被统计的词的一个子集。“重要的”词及其统计信息都存储在词汇表缓存中。重要和非重要的词之间并没有固定的界限，但区分这两类词的基本思路是：只出现一次（或者说出现少于五次）的词较难学习，可将其视为无益的噪声信号。

词汇表缓存保存着Word2vec和词袋等方法所需的元数据，这两种方法对于词的处理方式截然不同。Word2vec生成词的向量表示，亦称为神经词向量。词向量可以长至包含几百个系数，而这些系数帮助神经网络预测一个词在任何特定语境中的出现概率，比如在另一个特定的词之后出现的概率。以下是配置好的Word2vec：

<script src="http://gist-it.appspot.com/https://github.com/deeplearning4j/dl4j-0.0.3.3-examples/blob/master/src/main/java/org/deeplearning4j/word2vec/Word2VecRawTextExample.java?slice=58:74"></script>

获得词向量后，就可以将其输入一个深度神经网络来进行分类、预测、情感分析等操作了。
