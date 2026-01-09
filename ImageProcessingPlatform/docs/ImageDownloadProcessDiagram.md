# 图像下载和处理流程UML序列图

## 概述

该文档描述了图像处理平台中从服务器下载图像到最终显示的完整流程。

## UML序列图

```mermaid
sequenceDiagram
    participant UI as 用户界面 (MainForm)
    participant Config as 配置管理器 (ConfigManager)
    participant HttpClient as HTTP客户端 (HttpClient)
    participant Server as 服务器
    participant ImageProcess as 图像处理器 (CImageProcessByHamming)
    participant SDRaw as 图像数据 (CSD_Raw)
    participant Display as 图像显示

    Note over UI: 开始图像下载流程

    UI->>Config: 读取Config.xml配置
    Config-->>UI: 返回服务器地址和超时设置

    UI->>HttpClient: 发送GET请求 (DownloadPic)
    HttpClient->>Server: GET请求 (sampleId=GUID)
    Server-->>HttpClient: JSON响应 (code, Guid, ImgType, Imgs数组)
    HttpClient-->>UI: 返回响应结果

    alt 服务器响应失败
        UI->>Display: 显示加载失败信息
    else 服务器响应成功且GUID匹配
        UI->>SDRaw: 设置图像类型 (m_nLoadedMapType)
        
        UI->>HttpClient: 下载第一张图像
        HttpClient->>Server: 请求第一张图像
        Server-->>HttpClient: 返回图像数据
        HttpClient-->>UI: 图像数据
        
        UI->>UI: 保存第一张图像到m_mapLoadedMap[0]
        
        loop 下载剩余图像
            UI->>UI: 启动新线程LoadOnlinePicByThread
            UI->>HttpClient: 请求剩余图像
            HttpClient->>Server: 请求图像
            Server-->>HttpClient: 返回图像数据
            HttpClient-->>UI: 图像数据
            UI->>UI: 保存到m_mapLoadedMap[i]
        end

        UI->>ImageProcess: 创建CImageProcessByHamming对象
        UI->>SDRaw: 加载图像数据
        ImageProcess->>ImageProcess: ImageProcess1 (基本图像处理)
        ImageProcess->>ImageProcess: BrightAndContrastProcess (亮度对比度处理)
        ImageProcess-->>UI: 处理结果
        
        UI->>Display: ShowImgOnLabel显示图像
        Display->>Display: 转换为QImage格式
        Display->>Display: 按比例缩放显示
    end
```

## 详细流程说明

### 1. 配置读取阶段
- 系统从Config.xml中读取服务器地址、超时时间等配置信息

### 2. 服务器通信阶段
- 向服务器发送下载请求，参数包括当前的工作GUID
- 解析服务器返回的JSON数据

### 3. 图像下载阶段
- 验证返回的GUID与当前工作GUID是否匹配
- 下载第一张图像并保存到m_mapLoadedMap[0]
- 使用多线程下载其余图像

### 4. 图像处理阶段
- 使用CImageProcessByHamming类进行图像处理
- 执行基本图像处理和亮度对比度处理

### 5. 图像显示阶段
- 将处理后的图像转换为QImage格式
- 按比例缩放以适应显示区域