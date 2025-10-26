# Git代码管理指南

## 仓库信息

- **仓库名称**: zygl2 (资源管理系统)
- **主分支**: main
- **首次提交**: feat: 完成基于DDD的资源管理系统核心架构
- **提交ID**: 1fa6d76

## 基本配置

### 1. 配置用户信息（如果还未配置）

```bash
# 配置全局用户名和邮箱
git config --global user.name "你的名字"
git config --global user.email "your.email@example.com"

# 配置项目级用户名和邮箱（仅当前项目）
git config user.name "你的名字"
git config user.email "your.email@example.com"
```

### 2. 查看配置

```bash
# 查看所有配置
git config --list

# 查看特定配置
git config user.name
git config user.email
```

## 常用Git操作

### 日常开发流程

#### 1. 查看状态

```bash
# 查看当前状态
git status

# 查看简洁状态
git status -s
```

#### 2. 添加文件

```bash
# 添加特定文件
git add src/domain/new_file.h

# 添加特定目录
git add src/application/

# 添加所有修改的文件
git add .

# 交互式添加
git add -i
```

#### 3. 提交更改

```bash
# 提交并添加消息
git commit -m "feat: 添加新功能"

# 提交所有已跟踪文件的修改
git commit -am "fix: 修复bug"

# 修改最后一次提交
git commit --amend
```

#### 4. 查看历史

```bash
# 查看提交历史
git log

# 查看简洁历史
git log --oneline

# 查看图形化历史
git log --oneline --graph --all

# 查看特定文件的历史
git log -- src/domain/board.h

# 查看最近3次提交
git log -3

# 查看详细差异
git log -p
```

#### 5. 查看差异

```bash
# 查看工作区与暂存区的差异
git diff

# 查看暂存区与最后一次提交的差异
git diff --staged

# 查看特定文件的差异
git diff src/domain/board.h

# 查看两次提交之间的差异
git diff commit1 commit2
```

### 分支管理

#### 1. 创建和切换分支

```bash
# 创建新分支
git branch feature/new-feature

# 切换到分支
git checkout feature/new-feature

# 创建并切换到新分支（推荐）
git checkout -b feature/new-feature

# 使用新的switch命令（Git 2.23+）
git switch feature/new-feature
git switch -c feature/new-feature
```

#### 2. 查看分支

```bash
# 查看所有本地分支
git branch

# 查看所有分支（包括远程）
git branch -a

# 查看分支详细信息
git branch -v
```

#### 3. 合并分支

```bash
# 合并指定分支到当前分支
git merge feature/new-feature

# 创建合并提交（即使可以快进）
git merge --no-ff feature/new-feature

# 放弃合并
git merge --abort
```

#### 4. 删除分支

```bash
# 删除已合并的分支
git branch -d feature/old-feature

# 强制删除分支
git branch -D feature/old-feature
```

### 撤销操作

#### 1. 撤销工作区修改

```bash
# 撤销特定文件的修改
git checkout -- src/domain/board.h

# 使用restore命令（Git 2.23+）
git restore src/domain/board.h

# 撤销所有修改
git checkout -- .
git restore .
```

#### 2. 撤销暂存区的文件

```bash
# 取消暂存特定文件
git reset HEAD src/domain/board.h

# 使用restore命令（Git 2.23+）
git restore --staged src/domain/board.h
```

#### 3. 撤销提交

```bash
# 撤销最后一次提交，保留修改
git reset --soft HEAD^

# 撤销最后一次提交，取消暂存
git reset HEAD^

# 撤销最后一次提交，丢弃修改（危险！）
git reset --hard HEAD^

# 撤销到特定提交
git reset --hard commit_id
```

### 远程仓库操作

#### 1. 添加远程仓库

```bash
# 添加远程仓库
git remote add origin https://github.com/username/zygl2.git

# 查看远程仓库
git remote -v

# 显示远程仓库详细信息
git remote show origin
```

#### 2. 推送到远程

```bash
# 首次推送并设置上游分支
git push -u origin main

# 推送到远程
git push

# 推送特定分支
git push origin feature/new-feature

# 推送所有分支
git push --all

# 推送标签
git push --tags
```

#### 3. 从远程拉取

```bash
# 拉取并合并
git pull

# 拉取特定分支
git pull origin main

# 只拉取不合并
git fetch

# 查看远程分支
git fetch --all
```

#### 4. 克隆仓库

```bash
# 克隆仓库
git clone https://github.com/username/zygl2.git

# 克隆到指定目录
git clone https://github.com/username/zygl2.git my-project

# 浅克隆（只克隆最近的提交）
git clone --depth 1 https://github.com/username/zygl2.git
```

### 标签管理

#### 1. 创建标签

```bash
# 创建轻量标签
git tag v1.0.0

# 创建带注释的标签
git tag -a v1.0.0 -m "版本1.0.0 - 核心架构完成"

# 为特定提交打标签
git tag -a v1.0.0 commit_id -m "版本1.0.0"
```

#### 2. 查看标签

```bash
# 查看所有标签
git tag

# 查看标签详细信息
git show v1.0.0

# 查看特定模式的标签
git tag -l "v1.*"
```

#### 3. 删除标签

```bash
# 删除本地标签
git tag -d v1.0.0

# 删除远程标签
git push origin :refs/tags/v1.0.0
```

## 提交消息规范

### Conventional Commits规范

```
<类型>(<范围>): <描述>

[可选的正文]

[可选的脚注]
```

### 常用类型

- **feat**: 新功能
- **fix**: 修复bug
- **docs**: 文档更新
- **style**: 代码格式（不影响代码运行）
- **refactor**: 重构（既不是新功能也不是bug修复）
- **perf**: 性能优化
- **test**: 测试相关
- **chore**: 构建过程或辅助工具的变动

### 示例

```bash
# 新功能
git commit -m "feat(domain): 添加Board实体的新方法"

# 修复bug
git commit -m "fix(infrastructure): 修复双缓冲指针交换的竞态条件"

# 文档更新
git commit -m "docs: 更新README的安装说明"

# 性能优化
git commit -m "perf(application): 优化MonitoringService查询性能"

# 重构
git commit -m "refactor(interfaces): 重构UDP协议数据包结构"
```

## 推荐的分支策略

### Git Flow分支模型

```
main (生产分支)
  └─ develop (开发分支)
      ├─ feature/xxx (功能分支)
      ├─ bugfix/xxx (bug修复分支)
      └─ release/x.x.x (发布分支)
```

### 功能开发流程

```bash
# 1. 从develop创建功能分支
git checkout develop
git checkout -b feature/add-logging

# 2. 开发功能并提交
git add .
git commit -m "feat: 添加日志系统"

# 3. 完成后合并到develop
git checkout develop
git merge --no-ff feature/add-logging

# 4. 删除功能分支
git branch -d feature/add-logging
```

## 项目特定的Git工作流

### 1. 每日开发

```bash
# 开始工作前
git pull origin main

# 创建功能分支
git checkout -b feature/your-feature

# 开发并提交
git add src/application/services/new_service.h
git commit -m "feat(application): 添加新的应用服务"

# 定期推送到远程
git push -u origin feature/your-feature
```

### 2. 代码审查后合并

```bash
# 更新main分支
git checkout main
git pull origin main

# 合并功能分支
git merge --no-ff feature/your-feature

# 推送到远程
git push origin main

# 删除本地和远程分支
git branch -d feature/your-feature
git push origin --delete feature/your-feature
```

### 3. 处理冲突

```bash
# 如果出现冲突
git merge feature/your-feature
# Auto-merging src/domain/board.h
# CONFLICT (content): Merge conflict in src/domain/board.h

# 1. 手动解决冲突
# 编辑冲突文件，移除冲突标记

# 2. 添加解决后的文件
git add src/domain/board.h

# 3. 完成合并
git commit
```

## 有用的Git别名

### 配置别名

```bash
# 配置常用别名
git config --global alias.st status
git config --global alias.co checkout
git config --global alias.br branch
git config --global alias.ci commit
git config --global alias.lg "log --oneline --graph --all"
git config --global alias.last "log -1 HEAD"
git config --global alias.unstage "reset HEAD --"
git config --global alias.visual "log --oneline --graph --decorate --all"
```

### 使用别名

```bash
git st          # 等同于 git status
git co main     # 等同于 git checkout main
git br          # 等同于 git branch
git ci -m "msg" # 等同于 git commit -m "msg"
git lg          # 查看图形化日志
```

## 忽略文件

项目已配置`.gitignore`文件，包含以下内容：

- 编译输出文件（*.o, *.exe等）
- IDE配置文件（.vscode/, .idea/等）
- 构建目录（build/, bin/等）
- 日志文件（*.log）
- 临时文件（*.tmp, *.bak等）

### 添加更多忽略规则

编辑`.gitignore`文件：

```bash
# 编辑忽略文件
vim .gitignore

# 添加规则后提交
git add .gitignore
git commit -m "chore: 更新gitignore规则"
```

## 紧急情况处理

### 1. 恢复误删的文件

```bash
# 查找被删除的文件
git log --diff-filter=D --summary

# 恢复文件
git checkout commit_id^ -- path/to/file
```

### 2. 找回丢失的提交

```bash
# 查看引用日志
git reflog

# 恢复到特定提交
git reset --hard HEAD@{2}
```

### 3. 清理仓库

```bash
# 清理未跟踪的文件（先预览）
git clean -n

# 清理未跟踪的文件
git clean -f

# 清理未跟踪的文件和目录
git clean -fd
```

## 最佳实践

### 1. 提交频率
- ✅ 频繁提交，每个提交专注于一个改动
- ✅ 提交前确保代码可以编译
- ❌ 不要提交半完成的功能

### 2. 提交内容
- ✅ 每次提交应该是一个逻辑单元
- ✅ 提交消息应该清晰描述改动
- ❌ 不要在一次提交中混合多个不相关的改动

### 3. 分支管理
- ✅ 使用分支进行功能开发
- ✅ 定期合并主分支到功能分支
- ❌ 不要直接在main分支上开发

### 4. 代码审查
- ✅ 提交前先自己审查代码
- ✅ 使用Pull Request进行代码审查
- ✅ 解决所有审查意见后再合并

## 项目当前状态

```bash
# 查看当前状态
git status

# 查看提交历史
git log --oneline --graph --all

# 查看统计信息
git log --stat

# 查看代码贡献者
git shortlog -sn
```

## 参考资源

- [Pro Git Book](https://git-scm.com/book/zh/v2)
- [GitHub Guides](https://guides.github.com/)
- [Conventional Commits](https://www.conventionalcommits.org/)
- [Git Flow](https://nvie.com/posts/a-successful-git-branching-model/)

---

**最后更新**: 2025-10-26  
**Git版本**: 建议使用Git 2.23或更高版本

