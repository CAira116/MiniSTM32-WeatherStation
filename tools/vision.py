"""
豆包视觉识别工具 (Doubao Vision Tool)
======================================
基于火山引擎豆包大模型，支持本地图片和网络图片识别

使用方法:
    python vision.py <图片路径>
    python vision.py <图片路径> "你想问什么"
    python vision.py --setup  设置 API Key

示例:
    python vision.py photo.jpg
    python vision.py circuit.png "描述这张电路板的引脚排列"
    python vision.py screenshot.png "这个Keil报错是什么意思？"
"""

import os
import sys
import base64
import json
from pathlib import Path
from dotenv import load_dotenv, set_key

# ─── 配置 ──────────────────────────────────────────────
SCRIPT_DIR = Path(__file__).parent
ENV_FILE = SCRIPT_DIR / ".env"
API_ENDPOINT = "https://ark.cn-beijing.volces.com/api/v3"
DEFAULT_MODEL = None  # 从 .env 读取

# ─── 初始化 ────────────────────────────────────────────

def setup_api_key():
    """设置 API Key"""
    print("\n" + "=" * 50)
    print("  豆包视觉识别 - API Key 设置")
    print("=" * 50)
    print("""
获取 API Key 的步骤:

1. 打开 https://console.volcengine.com/ark
2. 注册/登录火山引擎 (用手机号)
3. 完成实名认证 (身份证 + 人脸, 5分钟)
4. 左侧菜单 → "API Key 管理" → "创建 API Key"
5. 复制生成的 Key 到下面

如果没有额度，先点 "开通管理" 开通豆包模型
新用户有 50万 tokens 免费额度
""")
    api_key = input("粘贴你的 API Key: ").strip()
    if api_key:
        set_key(str(ENV_FILE), "ARK_API_KEY", api_key)
        # 确保文件权限正确
        os.chmod(ENV_FILE, 0o600)
        print(f"\n[OK] API Key 已保存到 {ENV_FILE}")
        return api_key
    else:
        print("\n[X] 未输入 API Key")
        return None


def load_api_key():
    """加载 API Key"""
    load_dotenv(ENV_FILE)
    return os.environ.get("ARK_API_KEY")


# ─── 核心功能 ──────────────────────────────────────────

def image_to_base64(image_path: str) -> tuple:
    """
    将图片转换为 base64
    返回 (base64_string, mime_type)
    """
    ext = Path(image_path).suffix.lower()
    mime_map = {
        '.jpg':  'image/jpeg',
        '.jpeg': 'image/jpeg',
        '.png':  'image/png',
        '.gif':  'image/gif',
        '.webp': 'image/webp',
        '.bmp':  'image/bmp',
    }
    mime = mime_map.get(ext, 'image/jpeg')

    with open(image_path, 'rb') as f:
        data = base64.b64encode(f.read()).decode('utf-8')

    return data, mime


def analyze_image(image_path: str, question: str = None, api_key: str = None):
    """
    使用豆包视觉模型分析图片

    Args:
        image_path: 图片路径或URL
        question:  要问的问题，默认让模型自由描述
        api_key:   API Key

    Returns:
        模型的文字回复
    """
    if not api_key:
        raise ValueError("API Key 未设置，请先运行: python vision.py --setup")

    endpoint = os.environ.get("ARK_ENDPOINT")
    if not endpoint:
        raise ValueError("Endpoint 未设置，请检查 .env 文件")

    from volcenginesdkarkruntime import Ark

    # 默认问题
    if not question:
        question = "请详细描述这张图片的内容。如果是电路板、原理图或代码截图，请从专业角度分析。"

    # 判断是本地文件还是URL
    if image_path.startswith(('http://', 'https://')):
        image_content = {"url": image_path}
    else:
        if not os.path.exists(image_path):
            raise FileNotFoundError(f"图片不存在: {image_path}")
        b64_data, mime = image_to_base64(image_path)
        image_content = {"url": f"data:{mime};base64,{b64_data}"}

    # 初始化客户端
    client = Ark(
        base_url=API_ENDPOINT,
        api_key=api_key,
    )

    print(f"\n[Analyzing] 正在分析图片: {image_path}")
    print(f"[Q] 问题: {question}")
    print("-" * 40)

    # 调用API
    response = client.chat.completions.create(
        model=endpoint,
        messages=[
            {
                "role": "user",
                "content": [
                    {"type": "image_url", "image_url": image_content},
                    {"type": "text", "text": question},
                ],
            }
        ],
    )

    result = response.choices[0].message.content

    print(result)
    print("-" * 40)

    return result


# ─── 命令行入口 ────────────────────────────────────────

def main():
    if len(sys.argv) < 2 or sys.argv[1] in ('--help', '-h'):
        print(__doc__)
        return

    # --setup 模式
    if sys.argv[1] == '--setup':
        setup_api_key()
        return

    # --check 模式：检查配置
    if sys.argv[1] == '--check':
        api_key = load_api_key()
        if api_key:
            print(f"[OK] API Key 已配置 ({ENV_FILE})")
            print(f"   Key 前8位: {api_key[:8]}...")
        else:
            print(f"[X] API Key 未配置，运行 setup: python vision.py --setup")
        return

    # 正常分析模式
    image_path = sys.argv[1]
    question = sys.argv[2] if len(sys.argv) > 2 else None

    api_key = load_api_key()
    if not api_key:
        print("[X] 请先设置 API Key: python vision.py --setup")
        return

    try:
        analyze_image(image_path, question, api_key)
    except Exception as e:
        print(f"\n[X] 错误: {e}")
        print("\n可能的原因:")
        print("  1. API Key 无效或已过期 → 重新运行 --setup")
        print("  2. 未开通豆包模型 → 访问 https://console.volcengine.com/ark")
        print("  3. 额度用完了 → 检查控制台余额")
        print("  4. 网络不通 → 检查能否访问 ark.cn-beijing.volces.com")


if __name__ == '__main__':
    main()
