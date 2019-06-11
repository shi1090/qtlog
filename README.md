# qtlog
基于Qt日志的日志文本输出组件

重写日志消息处理函数，日志导出到文本，移植glog部分代码，扩展了分类导出功能。

## 导出模式
分级导出模式和分类导出模式

### 分级导出
支持QT日志的debug info warning error fatal 5级分类，不同等级日志分别导出到对应文件夹下，方便查阅

### 分类导出
配合QLoggingCategory类使用，设置Category后，分类模式下根据Category设置信息创建日志分类目录，支持多级目录

例如：

QLoggingCategory Category("msg.socket.105102")

日志将导出到 .../msg/socket/105102/ 地址下

分类模式下，同一分类下日志分级将不再区分，统一导出到同一日志下

## 日志分级规则
从Qt 5.3开始，日志记录规则也自动从日志配置文件的[rules]部分加载。

本组件采用QT_LOGGING_CONF环境变量配置方式，具体使用方式查看demo，配置完成后，后期可根据需求在配置文件中开启或关闭相关规则

## dump捕获
支持windows下dump捕获，程序崩溃生成最小dump记录，保存到指定文件夹下
