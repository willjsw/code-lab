#!/bin/sh
# download.sh — OSTEP 한국어 번역 PDF 53편 재수집
#
# 사용법:  sh download.sh
# 결과:    ./00-intro/ ./01-virtualization/ ./02-concurrency/ ./03-persistence/ 하위에
#          "<챕터번호>-<공식 표기 제목>.pdf" 형식으로 저장
#
# 파일명 규칙 — 공식 저장소 README 표의 제목을 그대로 사용.
#   공백은 하이픈으로, 파일명 사용 불가 문자('/' ':' '(' ')')는 제거
#   번호 없는 3편(Preface·Preface-Translate·TOC)은 00- 접두
#
# 출처 — https://github.com/remzi-arpacidusseau/ostep-translations/tree/master/korean

set -e

BASE="https://pages.cs.wisc.edu/~remzi/OSTEP/Korean"
DIR=$(dirname "$0")
cd "$DIR"

mkdir -p 00-intro 01-virtualization 02-concurrency 03-persistence

# 형식: <저장 디렉토리>|<원본 파일명>|<저장 파일명>
MAP=$(cat <<'MAPEOF'
00-intro|00-preface.pdf|00-Preface.pdf
00-intro|00-preface-tx.pdf|00-Preface-Translate.pdf
00-intro|00-toc.pdf|00-TOC.pdf
00-intro|01-dialogue-threeeasy.pdf|01-Dialogue.pdf
00-intro|02-intro.pdf|02-Introduction.pdf
01-virtualization|03-dialogue-virtualization.pdf|03-Dialogue.pdf
01-virtualization|04-cpu-intro.pdf|04-Processes.pdf
01-virtualization|05-cpu-api.pdf|05-Process-API.pdf
01-virtualization|06-cpu-mechanisms.pdf|06-Direct-Execution.pdf
01-virtualization|07-cpu-sched.pdf|07-CPU-Scheduling.pdf
01-virtualization|08-cpu-sched-mlfq.pdf|08-Multi-level-Feedback.pdf
01-virtualization|09-cpu-sched-lottery.pdf|09-Lottery-Scheduling.pdf
01-virtualization|10-cpu-sched-multi.pdf|10-Multi-CPU-Scheduling.pdf
01-virtualization|11-cpu-dialogue.pdf|11-Summary.pdf
01-virtualization|12-dialogue-vm.pdf|12-Dialogue.pdf
01-virtualization|13-vm-intro.pdf|13-Address-Spaces.pdf
01-virtualization|14-vm-api.pdf|14-Memory-API.pdf
01-virtualization|15-vm-mechanism.pdf|15-Address-Translation.pdf
01-virtualization|16-vm-segmentation.pdf|16-Segmentation.pdf
01-virtualization|17-vm-freespace.pdf|17-Free-Space-Management.pdf
01-virtualization|18-vm-paging.pdf|18-Introduction-to-Paging.pdf
01-virtualization|19_vm-tlbs.pdf|19-Translation-Lookaside-Buffers.pdf
01-virtualization|20_vm-smalltables.pdf|20-Advanced-Page-Tables.pdf
01-virtualization|21_vm-beyondphys.pdf|21-Swapping-Mechanisms.pdf
01-virtualization|22_vm-beyondphys-policy.pdf|22-Swapping-Policies.pdf
01-virtualization|23_vm-vax.pdf|23-Case-Study-VAX.pdf
01-virtualization|24_vm-dialogue.pdf|24-Summary.pdf
02-concurrency|25_dialogue-concurrency.pdf|25-Dialogue.pdf
02-concurrency|26_threads-intro.pdf|26-Concurrency-and-Threads.pdf
02-concurrency|27_threads-api.pdf|27-Thread-API.pdf
02-concurrency|28_threads-locks.pdf|28-Locks.pdf
02-concurrency|29_threads-locks-usage.pdf|29-Locked-Data-Structures.pdf
02-concurrency|30_threads-cv.pdf|30-Condition-Variables.pdf
02-concurrency|31_threads-sema.pdf|31-Semaphores.pdf
02-concurrency|32_threads-bugs.pdf|32-Concurrency-Bugs.pdf
02-concurrency|33_threads-events.pdf|33-Event-based-Concurrency.pdf
02-concurrency|34_threads_dialogue.pdf|34-Summary.pdf
03-persistence|35_dialogue-persistence.pdf|35-Dialogue.pdf
03-persistence|36_file-devices.pdf|36-IO-Devices.pdf
03-persistence|37_file_disks.pdf|37-Hard-Disk-Drives.pdf
03-persistence|38_RAID.pdf|38-Redundant-Disk-Arrays-RAID.pdf
03-persistence|39_interlude-file-directory.pdf|39-Files-and-Directories.pdf
03-persistence|40_FS-implementation.pdf|40-File-System-Implementation.pdf
03-persistence|41_FFS.pdf|41-Fast-File-System-FFS.pdf
03-persistence|42_crash-consistency.pdf|42-FSCK-and-Journaling.pdf
03-persistence|43_LFS.pdf|43-Log-Structured-File-System-LFS.pdf
03-persistence|44_data-integrity.pdf|44-Data-Integrity-and-Protection.pdf
03-persistence|45_file-dialogue.pdf|45-Summary.pdf
03-persistence|46_dialogue-distribution.pdf|46-Dialogue.pdf
03-persistence|47_dist-intro.pdf|47-Distributed-Systems.pdf
03-persistence|48_NFS.pdf|48-Network-File-System-NFS.pdf
03-persistence|49_AFS.pdf|49-Andrew-File-System-AFS.pdf
03-persistence|50_dist-dialogue.pdf|50-Summary.pdf
MAPEOF
)

ok=0
fail=0
echo "$MAP" | while IFS='|' read -r dir src dst; do
    [ -n "$dir" ] || continue
    if [ -s "$dir/$dst" ]; then
        echo "skip  $dir/$dst"
        continue
    fi
    if curl -sS -f -o "$dir/$dst" "$BASE/$src"; then
        echo "ok    $dir/$dst"
    else
        rm -f "$dir/$dst"
        echo "FAIL  $dir/$dst  (원본: $src)"
    fi
done

echo
echo "총 $(find . -name '*.pdf' | wc -l | tr -d ' ') / 53 편 확보"

# PDF 헤더 검증 — HTML 에러 페이지가 저장된 경우 탐지
find . -name '*.pdf' | while read -r f; do
    head -c 4 "$f" | grep -q '%PDF' || echo "BAD (PDF 아님): $f"
done
