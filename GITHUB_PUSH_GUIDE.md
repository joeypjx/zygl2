# GitHub推送指南

## 当前状态

✅ 远程仓库已添加：https://github.com/joeypjx/zygl2.git  
⏳ 等待身份认证后推送代码

## 推送前的准备

### 方法1：使用GitHub Personal Access Token (推荐)

#### 1. 创建Personal Access Token

1. 访问GitHub设置：https://github.com/settings/tokens
2. 点击 "Generate new token" -> "Generate new token (classic)"
3. 设置Token信息：
   - Note: `zygl2-project` (给这个token起个名字)
   - Expiration: 选择过期时间（建议90天或自定义）
   - Select scopes: 勾选以下权限
     - ✅ `repo` (完整的仓库访问权限)
4. 点击 "Generate token"
5. **重要**：复制生成的token（只显示一次！）

#### 2. 使用Token推送

```bash
# 方式A：使用token作为密码（推荐）
git push -u origin main
# 当提示输入用户名时：输入你的GitHub用户名 (joeypjx)
# 当提示输入密码时：粘贴刚才复制的Personal Access Token

# 方式B：在URL中包含token（不推荐，会暴露token）
git remote set-url origin https://YOUR_TOKEN@github.com/joeypjx/zygl2.git
git push -u origin main
```

#### 3. 保存凭据（避免每次输入）

```bash
# macOS使用Keychain保存凭据
git config --global credential.helper osxkeychain

# 第一次推送时输入token，之后会自动保存
git push -u origin main
```

---

### 方法2：使用SSH密钥（更安全）

#### 1. 生成SSH密钥

```bash
# 生成新的SSH密钥
ssh-keygen -t ed25519 -C "your.email@example.com"

# 按Enter使用默认位置 (~/.ssh/id_ed25519)
# 可以设置密码短语（推荐）或直接按Enter跳过

# 启动ssh-agent
eval "$(ssh-agent -s)"

# 添加SSH密钥到ssh-agent
ssh-add ~/.ssh/id_ed25519
```

#### 2. 添加SSH密钥到GitHub

```bash
# 复制公钥到剪贴板
pbcopy < ~/.ssh/id_ed25519.pub

# 或者查看公钥内容
cat ~/.ssh/id_ed25519.pub
```

然后：
1. 访问：https://github.com/settings/keys
2. 点击 "New SSH key"
3. Title: `zygl2-macbook` (给这个密钥起个名字)
4. Key: 粘贴刚才复制的公钥
5. 点击 "Add SSH key"

#### 3. 测试SSH连接

```bash
# 测试SSH连接
ssh -T git@github.com

# 应该看到：
# Hi joeypjx! You've successfully authenticated, but GitHub does not provide shell access.
```

#### 4. 更改远程URL为SSH格式

```bash
# 更改远程仓库URL为SSH格式
git remote set-url origin git@github.com:joeypjx/zygl2.git

# 验证更改
git remote -v

# 推送代码
git push -u origin main
```

---

### 方法3：使用GitHub CLI（最简单）

#### 1. 安装GitHub CLI

```bash
# macOS使用Homebrew安装
brew install gh

# 或者下载安装包
# https://github.com/cli/cli/releases
```

#### 2. 登录GitHub

```bash
# 登录GitHub账户
gh auth login

# 选择选项：
# - What account do you want to log into? -> GitHub.com
# - What is your preferred protocol for Git operations? -> HTTPS
# - Authenticate Git with your GitHub credentials? -> Yes
# - How would you like to authenticate GitHub CLI? -> Login with a web browser
```

#### 3. 推送代码

```bash
# 直接推送
git push -u origin main
```

---

## 推送步骤

### 完成身份认证后执行：

```bash
# 1. 确认当前状态
git status
git log --oneline --graph --all

# 2. 推送到远程仓库
git push -u origin main

# 3. 验证推送成功
git remote show origin
```

### 推送成功后，你应该能在GitHub上看到：

- ✅ 28个代码文件
- ✅ 11个文档文件
- ✅ 2次提交记录
- ✅ 完整的项目结构

---

## 推送后的操作

### 1. 创建README徽章（可选）

在README.md顶部添加：

```markdown
# 资源管理系统 (ZYGL2)

![GitHub stars](https://img.shields.io/github/stars/joeypjx/zygl2)
![GitHub forks](https://img.shields.io/github/forks/joeypjx/zygl2)
![GitHub issues](https://img.shields.io/github/issues/joeypjx/zygl2)
![GitHub license](https://img.shields.io/github/license/joeypjx/zygl2)
```

### 2. 设置仓库描述

1. 访问：https://github.com/joeypjx/zygl2
2. 点击 "⚙️ Settings"
3. 在 "About" 部分添加：
   - Description: `基于DDD的分布式计算系统运维监控管理平台`
   - Website: (可选)
   - Topics: `cpp`, `ddd`, `architecture`, `monitoring`, `distributed-systems`

### 3. 保护main分支（建议）

1. 访问：https://github.com/joeypjx/zygl2/settings/branches
2. 点击 "Add rule"
3. Branch name pattern: `main`
4. 勾选：
   - ✅ Require pull request reviews before merging
   - ✅ Require status checks to pass before merging
5. 点击 "Create"

### 4. 设置.gitignore和License

仓库已包含`.gitignore`，但可以考虑添加License：

```bash
# 创建MIT License（示例）
cat > LICENSE << 'EOF'
MIT License

Copyright (c) 2025 joeypjx

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
EOF

git add LICENSE
git commit -m "docs: 添加MIT License"
git push origin main
```

---

## 日常推送流程

### 开发新功能

```bash
# 1. 创建功能分支
git checkout -b feature/main-program

# 2. 开发并提交
git add src/main.cpp
git commit -m "feat: 添加主程序入口"

# 3. 推送功能分支到远程
git push -u origin feature/main-program

# 4. 在GitHub上创建Pull Request

# 5. 审查通过后合并到main
git checkout main
git pull origin main
git merge feature/main-program
git push origin main

# 6. 删除功能分支
git branch -d feature/main-program
git push origin --delete feature/main-program
```

### 拉取最新代码

```bash
# 拉取远程最新代码
git pull origin main

# 或者先fetch再merge
git fetch origin
git merge origin/main
```

---

## 常见问题

### Q: 推送失败："remote: Permission denied"

**解决方案**：
- 检查GitHub用户名是否正确
- 确认Personal Access Token有正确的权限
- 或者使用SSH密钥认证

### Q: 推送失败："Updates were rejected"

**解决方案**：
```bash
# 先拉取远程更改
git pull origin main --rebase

# 再推送
git push origin main
```

### Q: 忘记Personal Access Token

**解决方案**：
- 重新创建一个新的Token
- 或者切换到SSH认证方式

### Q: 推送大文件失败

**解决方案**：
```bash
# GitHub限制单个文件最大100MB
# 使用Git LFS管理大文件
brew install git-lfs
git lfs install
git lfs track "*.so"
git lfs track "*.dll"
```

---

## 快速命令参考

```bash
# 查看远程仓库
git remote -v

# 推送到远程
git push origin main

# 拉取远程更新
git pull origin main

# 查看远程分支
git branch -r

# 推送所有分支
git push --all origin

# 推送标签
git push --tags

# 强制推送（危险！）
git push -f origin main
```

---

## 团队协作建议

### 1. 代码审查流程

```bash
# 提交者
git checkout -b feature/xxx
# ... 开发 ...
git push -u origin feature/xxx
# 在GitHub上创建Pull Request

# 审查者
git fetch origin
git checkout feature/xxx
# 审查代码，提出建议

# 合并者
# 在GitHub上点击 "Merge pull request"
```

### 2. Issue跟踪

在GitHub Issues中跟踪：
- Bug报告
- 功能请求
- 待办事项

### 3. 项目管理

使用GitHub Projects：
- 创建看板
- 跟踪进度
- 分配任务

---

**准备好后，执行推送命令，你的代码就会出现在GitHub上！**

仓库地址：https://github.com/joeypjx/zygl2

